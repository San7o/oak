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

#include <chrono>
#include <ctime>
#include <deque>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <print>
#include <string>
#include <thread>
#include <unistd.h>

#ifdef OAK_USE_SOCKETS
#include <arpa/inet.h>
#include <cstring>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#define OAK_DEBUG(...) oak::log(oak::level::debug, __VA_ARGS__)
#define OAK_INFO(...) oak::log(oak::level::info, __VA_ARGS__)
#define OAK_WARNING(...) oak::log(oak::level::warning, __VA_ARGS__)
#define OAK_ERROR(...) oak::log(oak::level::error, __VA_ARGS__)
#define OAK_OUTPUT(...) oak::log(oak::level::output, __VA_ARGS__)

namespace oak
{

enum class level
{
    debug = 0,
    info,
    warning,
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
};

struct logger
{
    static long unsigned int flag_bits;
    static bool json_serialize;
    static level log_level;
    static std::ofstream log_file;
    static std::deque<std::string> log_queue;
#ifdef OAK_USE_SOCKETS
    static int log_socket;
#endif
};

long unsigned int logger::flag_bits = 1;
bool logger::json_serialize = false;
oak::level logger::log_level = oak::level::warning;
std::ofstream logger::log_file;
std::deque<std::string> logger::log_queue;
int logger::log_socket = -1;

inline level constexpr get_level()
{
    return logger::log_level;
}

inline long unsigned int constexpr get_flags()
{
    return logger::flag_bits;
}

inline bool constexpr is_open()
{
    return logger::log_file.is_open();
}

inline void constexpr set_level(const oak::level &lvl)
{
    logger::log_level = lvl;
}

void constexpr set_file(const std::string &file)
{
    logger::log_file.open(file, std::ios::app);
    if (!logger::log_file.is_open())
    {
        std::cout << "Error: Could not open log file" << std::endl;
    }
}

void constexpr set_json_serialization(bool json)
{
    logger::json_serialize = json;
}

bool constexpr get_json_serialization()
{
    return logger::json_serialize;
}

void constexpr close_file()
{
    if (is_open())
        logger::log_file.close();
}

#ifdef OAK_USE_SOCKETS
void constexpr close_socket()
{
    if (logger::log_socket > 0)
        close(logger::log_socket);
}
#endif

template <typename... Args> void constexpr set_flags(flags flg, Args... args)
{
    logger::flag_bits = 0;
    add_flags(flg, args...);
}

template <typename... Args> void constexpr add_flags(flags flg, Args... args)
{
    logger::flag_bits |= static_cast<long unsigned int>(flg);
    if constexpr (sizeof...(args) > 0)
    {
        add_flags(args...);
    }
}

// Base case
void constexpr add_flags()
{
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
            else if (value == "warning")
                set_level(level::warning);
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
            else
                return std::unexpected("Invalid flags in file");
        }
        else if (key == "json")
        {
            if (value == "true")
                set_json_serialization(true);
            else if (value == "false")
                set_json_serialization(false);
            else
                return std::unexpected("Invalid json value in file");
        }
        else if (key == "file")
        {
            set_file(value);
        }
        else
        {
            return std::unexpected("Invalid key in file");
        }
    }

    return 0;
}

template <typename... Args>
std::string log_to_string(const level &lvl, const std::string &fmt,
                          Args &&...args)
{
    auto flags = get_flags();
    auto json = get_json_serialization();
    std::string prefix = "";
    if (flags > 0 && !json)
    {
            prefix += "[";
    }
    if (json)
    {
        prefix += "{ ";
    }
    if (flags & static_cast<long unsigned int>(flags::level))
    {
        if (json)
            prefix += std::format("\"level\": \"{}\"", lvl);
        else
            prefix += std::format("level={}", lvl);
    }
    if (flags & static_cast<long unsigned int>(flags::date))
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time_t);
        std::ostringstream oss;
        if (json)
            oss << ", \"date\": \"" << std::put_time(&now_tm, "%Y-%m-%d") << "\"";
        else
            oss << ",date=" << std::put_time(&now_tm, "%Y-%m-%d");
        prefix += oss.str();
    }
    if (flags & static_cast<long unsigned int>(flags::time))
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time_t);
        std::ostringstream oss;
        if (json)
            oss << ", \"time\": \"" << std::put_time(&now_tm, "%H:%M:%S") << "\"";
        else
            oss << ",time=" << std::put_time(&now_tm, "%H:%M:%S");
        prefix += oss.str();
    }
    if (flags & static_cast<long unsigned int>(flags::pid))
    {
        if (json)
            prefix += ", \"pid\": " + std::to_string(getpid());
        else
            prefix += std::format(",pid={}", getpid());
    }
    if (flags & static_cast<long unsigned int>(flags::tid))
    {
        if (json)
            prefix += ", \"tid\": " + std::to_string(
                std::hash<std::thread::id>{}(std::this_thread::get_id()));
        else
            prefix += std::format(",tid={}", std::this_thread::get_id());
    }

    if (flags > 0 && !json)
        prefix += "] ";
    if (flags > 0 && json)
        prefix += ", ";
    std::string formatted_string =
        std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
    if (json)
        return std::format("{}\"message\": \"{}\" }}\n", prefix, formatted_string);
    return std::format("{}{}\n", prefix, formatted_string);
}

template <typename... Args>
void log_to_stdout(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl)
        return;
    std::cout << log_to_string(lvl, fmt, args...);
}

inline void log_to_stdout(const std::string &str)
{
    std::cout << str;
}

template <typename... Args>
void log_to_file(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl && !logger::log_file.is_open())
        return;
    logger::log_file << log_to_string(lvl, fmt, args...);
}

void log_to_file(const std::string &str)
{
    if (logger::log_file.is_open())
        logger::log_file << str;
}

#ifdef OAK_USE_SOCKETS
template <typename... Args>
void log_to_socket(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl || logger::log_socket < 0)
        return;
    std::string formatted_string = log_to_string(lvl, fmt, args...);
    write(logger::log_socket, formatted_string.c_str(),
          formatted_string.size());
}

void log_to_socket(const std::string &str)
{
    if (logger::log_socket > 0)
        write(logger::log_socket, str.c_str(), str.size());
}
#endif

template <typename... Args>
void log(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl)
        return;
    std::string formatted_string = log_to_string(lvl, fmt, args...);
    log_to_stdout(formatted_string);
    if (logger::log_file.is_open())
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
inline void warning(const std::string &fmt, Args &&...args)
{
    log(oak::level::warning, fmt, args...);
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

void flush()
{
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
        case oak::level::warning:
            return format_to(ctx.out(), "warning");
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
