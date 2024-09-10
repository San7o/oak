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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <print>
#include <string>
#include <expected>

// Socket
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

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

class logger
{
public:
    static level log_level;
    static int log_socket;
    static std::ofstream log_file;
    static std::deque<std::string> log_queue;
};

oak::level logger::log_level = oak::level::warning;
std::ofstream logger::log_file;
std::deque<std::string> logger::log_queue;
int logger::log_socket = -1;

level constexpr get_level()
{
    return logger::log_level;
}

bool constexpr is_open()
{
    return logger::log_file.is_open();
}

void constexpr set_level(const oak::level &lvl)
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

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    logger::log_file << "----------" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
             << "----------" << std::endl;
}

template <typename... Args>
void log_to_stdout(const level &lvl,
                           const std::format_string<Args...> &fmt, Args... args)
{
    if (get_level() > lvl)
        return;
    std::string formatted_string = std::format(fmt, args...);
    std::cout << std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void log_to_file(const level &lvl,
                         const std::format_string<Args...> &fmt, Args... args)
{
    if (get_level() > lvl && !logger::log_file.is_open())
        return;
    std::string formatted_string = std::format(fmt, args...);
    logger::log_file << std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void log_to_socket(const level &lvl,
                           const std::format_string<Args...> &fmt, Args... args)
{
    if (get_level() > lvl || logger::log_socket < 0)
        return;
    std::cout << "socket" << std::endl << std::flush;
    std::string formatted_string = std::format(fmt, args...);
    write(logger::log_socket, formatted_string.c_str(), formatted_string.size());
}

template <typename... Args>
void log(const level &lvl, const std::format_string<Args...> &fmt,
                 Args... args)
{
    if (get_level() > lvl)
        return;
    log_to_stdout(lvl, fmt, args...);
    if (logger::log_file.is_open())
        log_to_file(lvl, fmt, args...);
    if (logger::log_socket > 0)
        log_to_socket(lvl, fmt, args...);
}

#ifdef __unix__
[[nodiscard]] std::expected<int, std::string> set_socket(const std::string& sock_addr)
{
    if (sock_addr.size() > 108)
    {
        return std::unexpected("Socket address too long, max 108 characters");
    }

    if(logger::log_socket > 0)
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
    if (connect(logger::log_socket, (struct sockaddr *)&sockaddr_un, sizeof(sockaddr_un)) < 0)
    {
        return std::unexpected("Could not connect to socket");
    }

    return logger::log_socket;
}
#endif

void constexpr close_file()
{
    if (is_open())
        logger::log_file.close();
}

template <typename... Args>
void out(const std::format_string<Args...> &fmt, Args... args)
{
    log(oak::level::output, fmt, args...);
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
            return format_to(ctx.out(), "ERROR");
        case oak::level::warning:
            return format_to(ctx.out(), "WARNING");
        case oak::level::info:
            return format_to(ctx.out(), "INFO");
        case oak::level::debug:
            return format_to(ctx.out(), "DEBUG");
        case oak::level::output:
            return format_to(ctx.out(), "OUTPUT");
        default:
            return format_to(ctx.out(), "UNKNOWN");
        }
    }
};
