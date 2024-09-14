/*
 * MIT License
 *
 * Copyright (c) 2024 Giovanni Santini

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "oak/oak.hpp"

using namespace oak;

long unsigned int oak::logger::flag_bits = 1;
oak::level oak::logger::log_level = oak::level::warn;
std::ofstream oak::logger::log_file;
std::deque<queue_element> oak::logger::log_queue;
std::mutex oak::logger::log_mutex;
std::condition_variable oak::logger::log_cv;
std::atomic<bool> oak::logger::close_writer = false;
std::optional<std::jthread> oak::logger::writer_thread;
#ifdef OAK_USE_SOCKETS
int oak::logger::log_socket = -1;
#endif

[[nodiscard]] std::expected<int, std::string> oak::set_file(const std::string &file)
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (logger::log_file.is_open())
    {
        logger::log_file.close();
    }
    if (!std::filesystem::exists(file))
    {
        return std::unexpected("File does not exist");
    }
    logger::log_file.open(file, std::ios::app);
    if (!logger::log_file.is_open())
    {
        return std::unexpected("Could not open log file");
    }
    if (!logger::log_file.good())
    {
        return std::unexpected("Error opening log file");
    }
    return 0;
}

void oak::close_file()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (logger::log_file.is_open())
        logger::log_file.close();
}

#ifdef OAK_USE_SOCKETS
void oak::close_socket()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (logger::log_socket > 0)
        close(logger::log_socket);
}
#endif

void oak::add_to_queue(const std::string &str, const destination &d)
{
    {
        std::lock_guard<std::mutex> lock(logger::log_mutex);
        logger::log_queue.push_back({str, d});
    }
    logger::log_cv.notify_one();
}

void oak::writer()
{
    while (!logger::close_writer.load())
    {
        std::unique_lock<std::mutex> lock(logger::log_mutex);
        logger::log_cv.wait(lock,
                            [] {
                                return !logger::log_queue.empty()
                                       || logger::close_writer.load();
                            });
        while (!logger::log_queue.empty())
        {
            auto elem = logger::log_queue.front();
            logger::log_queue.pop_front();
            switch (elem.dest)
            {
            case oak::destination::std_out:
                std::cout << elem.message;
                break;
            case oak::destination::file:
                logger::log_file << elem.message;
                break;
            case oak::destination::socket:
#ifdef OAK_USE_SOCKETS
                write(logger::log_socket, elem.message.c_str(),
                      elem.message.size());
#endif
                break;
            default:
                break;
            }
        }
    }
}

void oak::init_writer()
{
    logger::writer_thread.emplace([] { writer(); });
}

void oak::stop_writer()
{
    logger::close_writer = true;
    logger::log_cv.notify_one();
    logger::writer_thread->join();
}

[[nodiscard]] std::expected<int, std::string> oak::settings_file(
    const std::string &file)
{
    if (file.size() == 0)
        return std::unexpected("Settings file path is empty");

    if (!std::filesystem::exists(file))
    {
        return std::unexpected("Settings file does not exist");
    }

    std::ifstream settings(file);
    while (!settings.eof())
    {
        std::string line;
        std::getline(settings, line);
        if (line.size() == 0)
            continue;

        std::string key = line.substr(0, line.find('='));
        std::string value = line.substr(line.find('=') + 1);

        key.erase(std::remove_if(key.begin(), key.end(), isspace), key.end());
        value.erase(std::remove_if(value.begin(), value.end(), isspace),
                    value.end());

        if (key == "level")
        {
            if (value == "debug")
                set_level(level::debug);
            else if (value == "info")
                set_level(level::info);
            else if (value == "warn")
                set_level(level::warn);
            else if (value == "error")
                set_level(level::error);
            else if (value == "output")
                set_level(level::output);
            else
                return std::unexpected("Invalid log level in file");
        }
        else if (key == "flags")
        {
            set_flags(flags::none);
            while (value.find(',') != std::string::npos)
            {
                std::string flag = value.substr(0, value.find(','));
                value = value.substr(value.find(',') + 1);
                if (flag == "none")
                    add_flags(flags::none);
                else if (flag == "level")
                    add_flags(flags::level);
                else if (flag == "date")
                    add_flags(flags::date);
                else if (flag == "time")
                    add_flags(flags::time);
                else if (flag == "pid")
                    add_flags(flags::pid);
                else if (flag == "tid")
                    add_flags(flags::tid);
                else if (flag == "json")
                    add_flags(flags::json);
                else
                    return std::unexpected("Invalid flags in file");
            }
            // get last element
            if (value == "none")
                add_flags(flags::none);
            else if (value == "level")
                add_flags(flags::level);
            else if (value == "date")
                add_flags(flags::date);
            else if (value == "time")
                add_flags(flags::time);
            else if (value == "pid")
                add_flags(flags::pid);
            else if (value == "tid")
                add_flags(flags::tid);
            else if (value == "json")
                add_flags(flags::json);
            else
                return std::unexpected("Invalid flags in file");
        }
        else if (key == "file")
        {
            auto exp = set_file(value);
            if (!exp.has_value())
            {
                return std::unexpected("Could not open file");
            }
        }
        else
        {
            return std::unexpected("Invalid key in file");
        }
    }

    return 0;
}

void oak::log_to_file(const std::string &str)
{
    if (is_file_open())
        add_to_queue(str, oak::destination::file);
}

#ifdef OAK_USE_SOCKETS
void oak::log_to_socket(const std::string &str)
{
    if (logger::log_socket > 0)
        add_to_queue(str, oak::destination::socket);
}
#endif

#ifdef OAK_USE_SOCKETS
#ifdef __unix__
[[nodiscard]] std::expected<int, std::string>
oak::set_socket(const std::string &sock_addr)
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (sock_addr.size() > 108)
    {
        return std::unexpected("Socket address too long, max 108 characters");
    }

    if (logger::log_socket > 0)
    {
        close(logger::log_socket);
    }

    logger::log_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (logger::log_socket < 0)
    {
        return std::unexpected("Could not create socket");
    }

    struct sockaddr_un sockaddr_un;
    sockaddr_un.sun_family = AF_UNIX;
    strcpy(sockaddr_un.sun_path, sock_addr.c_str());
    if (connect(logger::log_socket, (struct sockaddr *) &sockaddr_un,
                sizeof(sockaddr_un))
        < 0)
    {
        return std::unexpected("Could not connect to socket");
    }

    return logger::log_socket;
}

[[nodiscard]] std::expected<int, std::string>
oak::set_socket(const std::string &addr, short unsigned int port,
           const protocol_t &protocol)
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (logger::log_socket > 0)
    {
        close(logger::log_socket);
    }

    switch (protocol)
    {
    case protocol_t::tcp:
        logger::log_socket = socket(AF_INET, SOCK_STREAM, 0);
        break;
    case protocol_t::udp:
        logger::log_socket = socket(AF_INET, SOCK_DGRAM, 0);
        break;
    default:
        return std::unexpected("Invalid protocol");
    };
    logger::log_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (logger::log_socket < 0)
    {
        return std::unexpected("Could not create socket");
    }

    struct sockaddr_in sockaddr_in;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(port);
    if (inet_pton(AF_INET, addr.c_str(), &sockaddr_in.sin_addr) <= 0)
    {
        return std::unexpected("Invalid address");
    }

    if (connect(logger::log_socket, (struct sockaddr *) &sockaddr_in,
                sizeof(sockaddr_in))
        < 0)
    {
        return std::unexpected("Could not connect to socket");
    }

    return logger::log_socket;
}
#endif
#endif

void oak::flush()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    std::cout << std::flush;
    if (logger::log_file.is_open())
        logger::log_file << std::flush;
}
