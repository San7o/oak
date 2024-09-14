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
    inline queue_element(const std::string &msg, const oak::destination &d)
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

[[nodiscard]]
std::expected<int, std::string> set_file(const std::string &file);
void close_file();

#ifdef OAK_USE_SOCKETS
void close_socket();
#endif

void add_to_queue(const std::string &str, const destination &d);

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

void writer();
void init_writer();
void stop_writer();

[[nodiscard]] std::expected<int, std::string> settings_file(
    const std::string &file);

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
    try {
        std::string formatted_string =
            std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        if (json)
            return std::vformat("{}\"message\": \"{}\" }}\n",
                                std::make_format_args(prefix, formatted_string));
        return std::vformat("{}{}\n",
                            std::make_format_args(prefix, formatted_string));
    } catch (const std::exception &e) {
        return "";
    }
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

void log_to_file(const std::string &str);

#ifdef OAK_USE_SOCKETS
template <typename... Args>
void log_to_socket(const level &lvl, const std::string &fmt, Args &&...args)
{
    if (get_level() > lvl || logger::log_socket < 0)
        return;
    std::string formatted_string = log_to_string(lvl, fmt, args...);
    add_to_queue(formatted_string, oak::destination::socket);
}

void log_to_socket(const std::string &str);
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
set_socket(const std::string &sock_addr);

[[nodiscard]] std::expected<int, std::string>
set_socket(const std::string &addr, short unsigned int port,
           const protocol_t &protocol = protocol_t::tcp);
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

void flush();

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
