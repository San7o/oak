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

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <print>
#include <string>
#include <thread>
#include <unistd.h>

#ifdef OAK_USE_SOCKETS
#include <cstring>
#include <netinet/ip.h>
#include <unistd.h>
#ifdef __unix__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif
#endif

#define OAK_DEBUG(...) oak::log(oak::level::debug, __VA_ARGS__)
#define OAK_INFO(...) oak::log(oak::level::info, __VA_ARGS__)
#define OAK_WARN(...) oak::log(oak::level::warn, __VA_ARGS__)
#define OAK_ERROR(...) oak::log(oak::level::error, __VA_ARGS__)
#define OAK_OUTPUT(...) oak::log(oak::level::output, __VA_ARGS__)

namespace oak
{

enum class level
{
    debug = 0,
    info,
    warn,
    error,
    output,
    disabled,
    _max_level
};

enum class protocol_t
{
    tcp = 0,
    udp,
    _max_protocol
};

enum class flags
{
    none = 0,
    level = 1,
    date = 2,
    time = 4,
    pid = 8,
    tid = 16,
    json = 32
};

enum class destination
{
    std_out = 0,
    file,
    socket,
    _max_destination
};

struct queue_element
{
    std::string message;
    oak::destination dest;
    queue_element(const std::string &msg, const oak::destination &d)
        : message(std::move(msg)), dest(d)
    {
    }
};

struct logger
{
    static long unsigned int flag_bits;
    static level log_level;
    static std::ofstream log_file;
    static std::deque<queue_element> log_queue;
    static std::mutex log_mutex;
    static std::condition_variable log_cv;
    static std::atomic<bool> close_writer;
    static std::optional<std::jthread> writer_thread;
#ifdef OAK_USE_SOCKETS
    static int log_socket;
#endif
};

long unsigned int logger::flag_bits = 1;
oak::level logger::log_level = oak::level::warn;
std::ofstream logger::log_file;
std::deque<queue_element> logger::log_queue;
std::mutex logger::log_mutex;
std::condition_variable logger::log_cv;
std::atomic<bool> logger::close_writer = false;
std::optional<std::jthread> logger::writer_thread;
#ifdef OAK_USE_SOCKETS
int logger::log_socket = -1;
#endif

inline level get_level()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    return logger::log_level;
}

inline long unsigned int get_flags()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    return logger::flag_bits;
}

inline bool is_file_open()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    return logger::log_file.is_open();
}

inline void set_level(const oak::level &lvl)
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    logger::log_level = lvl;
}

[[nodiscard]] std::expected<int, std::string> set_file(const std::string &file)
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

void close_file()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (logger::log_file.is_open())
        logger::log_file.close();
}

#ifdef OAK_USE_SOCKETS
void close_socket()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    if (logger::log_socket > 0)
        close(logger::log_socket);
}
#endif

void add_to_queue(const std::string &str, const destination &d)
{
    {
        std::lock_guard<std::mutex> lock(logger::log_mutex);
        logger::log_queue.push_back({str, d});
    }
    logger::log_cv.notify_one();
}

template <typename... Args> void add_flags(flags flg, Args &&...args)
{
    {
        std::lock_guard<std::mutex> lock(logger::log_mutex);
        logger::flag_bits |= static_cast<long unsigned int>(flg);
    }
    if (sizeof...(args) > 0)
    {
        add_flags(args...);
    }
}

// Base case
template <typename... Args> void add_flags(flags flg)
{
    {
        std::lock_guard<std::mutex> lock(logger::log_mutex);
        logger::flag_bits |= static_cast<long unsigned int>(flg);
    }
}

template <typename... Args> void set_flags(flags flg, Args &&...args)
{
    {
        std::lock_guard<std::mutex> lock(logger::log_mutex);
        logger::flag_bits = 0;
    }
    add_flags(flg, args...);
}

void writer()
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
                write(logger::log_socket, elem.message.c_str(),
                      elem.message.size());
                break;
            default:
                break;
            }
        }
    }
}

void init_writer()
{
    logger::writer_thread.emplace([] { writer(); });
}

void stop_writer()
{
    logger::close_writer = true;
    logger::log_cv.notify_one();
    logger::writer_thread->join();
}

[[nodiscard]] std::expected<int, std::string> constexpr settings_file(
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

template <typename... Args>
std::string constexpr log_to_string(const level &lvl, const std::string &fmt,
                                    Args &&...args)
{
    auto flags = get_flags();
    bool json = false;
    if (flags & static_cast<long unsigned int>(flags::json))
        json = true;
    std::string prefix = "";
    if (flags > 0 && !json)
    {
        prefix += "[ ";
    }
    if (json)
    {
        prefix += "{ ";
    }
    if (flags & static_cast<long unsigned int>(flags::level))
    {
        if (json)
            prefix +=
                std::vformat("\"level\": \"{}\"", std::make_format_args(lvl));
        else
            prefix += std::vformat("level={} ", std::make_format_args(lvl));
    }
    if (flags & static_cast<long unsigned int>(flags::date))
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time_t);
        std::ostringstream oss;
        if (json)
            oss << ", \"date\": \"" << std::put_time(&now_tm, "%Y-%m-%d")
                << "\"";
        else
            oss << "date=" << std::put_time(&now_tm, "%Y-%m-%d") << " ";
        prefix += oss.str();
    }
    if (flags & static_cast<long unsigned int>(flags::time))
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time_t);
        std::ostringstream oss;
        if (json)
            oss << ", \"time\": \"" << std::put_time(&now_tm, "%H:%M:%S")
                << "\"";
        else
            oss << "time=" << std::put_time(&now_tm, "%H:%M:%S") << " ";
        prefix += oss.str();
    }
    if (flags & static_cast<long unsigned int>(flags::pid))
    {
        std::string pid = std::to_string(getpid());
        if (json)
            prefix += ", \"pid\": " + pid;
        else
            prefix += std::vformat("pid={} ", std::make_format_args(pid));
    }
    if (flags & static_cast<long unsigned int>(flags::tid))
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        std::string tid = oss.str();
        if (json)
            prefix += ", \"tid\": " + tid;
        else
            prefix += std::vformat("tid={} ", std::make_format_args(tid));
    }

    if (flags - static_cast<long unsigned int>(flags::json) > 0 && !json)
        prefix += "] ";
    if (flags - static_cast<long unsigned int>(flags::json) > 0 && json)
        prefix += ", ";
    std::string formatted_string =
        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
    if (json)
        return std::vformat("{}\"message\": \"{}\" }}\n",
                            std::make_format_args(prefix, formatted_string));
    return std::vformat("{}{}\n",
                        std::make_format_args(prefix, formatted_string));
}

template <typename... Args>
void log_to_stdout(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl)
        return;
    std::string message = log_to_string(lvl, fmt, args...);
    add_to_queue(message, oak::destination::std_out);
}

inline void log_to_stdout(const std::string &str)
{
    add_to_queue(str, oak::destination::std_out);
}

template <typename... Args>
void log_to_file(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl && !is_file_open())
        return;
    std::string message = log_to_string(lvl, fmt, args...);
    add_to_queue(message, oak::destination::file);
}

void log_to_file(const std::string &str)
{
    if (is_file_open())
        add_to_queue(str, oak::destination::file);
}

#ifdef OAK_USE_SOCKETS
template <typename... Args>
void log_to_socket(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl || logger::log_socket < 0)
        return;
    std::string formatted_string = log_to_string(lvl, fmt, args...);
    add_to_queue(formatted_string, oak::destination::socket);
}

void log_to_socket(const std::string &str)
{
    if (logger::log_socket > 0)
        add_to_queue(str, oak::destination::socket);
}
#endif

template <typename... Args>
void log(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl)
        return;
    std::string formatted_string = log_to_string(lvl, fmt, args...);
    log_to_stdout(formatted_string);
    if (is_file_open())
        log_to_file(formatted_string);
#ifdef OAK_USE_SOCKETS
    if (logger::log_socket > 0)
        log_to_socket(formatted_string);
#endif
}

#ifdef OAK_USE_SOCKETS
#ifdef __unix__
[[nodiscard]] std::expected<int, std::string>
set_socket(const std::string &sock_addr)
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
set_socket(const std::string &addr, short unsigned int port,
           const protocol_t &protocol = protocol_t::tcp)
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

template <typename... Args>
inline void out(const std::string &fmt, Args &&...args)
{
    log(oak::level::output, fmt, args...);
}

template <typename... Args>
inline void debug(const std::string &fmt, Args &&...args)
{
    log(oak::level::debug, fmt, args...);
}

template <typename... Args>
inline void info(const std::string &fmt, Args &&...args)
{
    log(oak::level::info, fmt, args...);
}

template <typename... Args>
inline void warn(const std::string &fmt, Args &&...args)
{
    log(oak::level::warn, fmt, args...);
}

template <typename... Args>
inline void error(const std::string &fmt, Args &&...args)
{
    log(oak::level::error, fmt, args...);
}

template <typename... Args>
inline void output(const std::string &fmt, Args &&...args)
{
    log(oak::level::output, fmt, args...);
}

template <typename... Args>
inline void async(const level &lvl, const std::string &fmt, Args &&...args)
{
    (void) std::async([lvl, fmt, args...]() { log(lvl, fmt, args...); });
}

void flush()
{
    std::lock_guard<std::mutex> lock(logger::log_mutex);
    std::cout << std::flush;
    if (logger::log_file.is_open())
        logger::log_file << std::flush;
}

} // namespace oak

template <> struct std::formatter<oak::level>
{
    constexpr auto parse(format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const oak::level &level, FormatContext &ctx) const
    {
        switch (level)
        {
        case oak::level::error:
            return format_to(ctx.out(), "error");
        case oak::level::warn:
            return format_to(ctx.out(), "warn");
        case oak::level::info:
            return format_to(ctx.out(), "info");
        case oak::level::debug:
            return format_to(ctx.out(), "debug");
        case oak::level::output:
            return format_to(ctx.out(), "output");
        default:
            return format_to(ctx.out(), "unknown");
        }
    }
};

template <> struct std::formatter<oak::flags>
{
    constexpr auto parse(format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const oak::flags &flags, FormatContext &ctx) const
    {
        switch (flags)
        {
        case oak::flags::none:
            return format_to(ctx.out(), "none");
        case oak::flags::level:
            return format_to(ctx.out(), "level");
        case oak::flags::date:
            return format_to(ctx.out(), "date");
        case oak::flags::time:
            return format_to(ctx.out(), "time");
        case oak::flags::pid:
            return format_to(ctx.out(), "pid");
        case oak::flags::tid:
            return format_to(ctx.out(), "tid");
        default:
            return format_to(ctx.out(), "unknown");
        }
    }
};
