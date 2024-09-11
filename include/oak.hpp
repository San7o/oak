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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <print>
#include <string>

#ifdef OAK_USE_SOCKETS
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
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

struct logger
{
    static level log_level;
    static std::ofstream log_file;
    static std::deque<std::string> log_queue;
#ifdef OAK_USE_SOCKETS
    static int log_socket;
#endif
};

oak::level logger::log_level = oak::level::warning;
std::ofstream logger::log_file;
std::deque<std::string> logger::log_queue;
int logger::log_socket = -1;

inline level constexpr get_level()
{
    return logger::log_level;
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

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    logger::log_file << "----------"
                     << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
                     << "----------" << std::endl;
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

// TODO: add more verbosity and options to set verbosity level
template<typename... Args>
std::string log_to_string(const level &lvl, const std::format_string<Args...> &fmt, Args... args)
{
    std::string formatted_string = std::format(fmt, args...);
    return std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void log_to_stdout(const level &lvl, const std::format_string<Args...> &fmt,
                   Args... args)
{
    if (get_level() > lvl)
        return;
    std::cout << log_to_string(lvl, fmt, args...);
}

inline void log_to_stdout(const std::string& str)
{
    std::cout << str;
}

template <typename... Args>
void log_to_file(const level &lvl, const std::format_string<Args...> &fmt,
                 Args... args)
{
    if (get_level() > lvl && !logger::log_file.is_open())
        return;
    logger::log_file << log_to_string(lvl, fmt, args...);
}

void log_to_file(const std::string& str)
{
    if (logger::log_file.is_open())
        logger::log_file << str;
}

#ifdef OAK_USE_SOCKETS
template <typename... Args>
void log_to_socket(const level &lvl, const std::format_string<Args...> &fmt,
                   Args... args)
{
    if (get_level() > lvl || logger::log_socket < 0)
        return;
    std::string formatted_string = log_to_string(lvl, fmt, args...);
    write(logger::log_socket, formatted_string.c_str(),
          formatted_string.size());
}

void log_to_socket(const std::string& str)
{
    if (logger::log_socket > 0)
        write(logger::log_socket, str.c_str(), str.size());
}
#endif

template <typename... Args>
void log(const level &lvl, const std::format_string<Args...> &fmt, Args... args)
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
                const protocol_t& protocol = protocol_t::tcp)
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
