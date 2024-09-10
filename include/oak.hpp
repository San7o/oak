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

#define OAK_DEBUG(...) oak::logger::log(oak::level::debug, __VA_ARGS__)
#define OAK_INFO(...) oak::logger::log(oak::level::info, __VA_ARGS__)
#define OAK_WARNING(...) oak::logger::log(oak::level::warning, __VA_ARGS__)
#define OAK_ERROR(...) oak::logger::log(oak::level::error, __VA_ARGS__)
#define OAK_OUTPUT(...) oak::logger::log(oak::level::output, __VA_ARGS__)

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
    static level constexpr get_level();
    static bool constexpr is_open();
    static void constexpr set_level(const level &lvl);
    static void constexpr set_log_file(const std::string &file);
    static void constexpr close_file();

    template <typename... Args>
    static void log_to_stdout(const level &lvl,
                              const std::format_string<Args...> &fmt,
                              Args... args);
    template <typename... Args>
    static void log_to_file(const level &lvl,
                            const std::format_string<Args...> &fmt,
                            Args... args);
    template <typename... Args>
    static void log(const level &lvl, const std::format_string<Args...> &fmt,
                    Args... args);

  private:
    static level log_level;
    static std::ofstream log_file;
    static std::deque<std::string> log_queue;
};

oak::level logger::log_level = oak::level::warning;
std::ofstream logger::log_file;
std::deque<std::string> logger::log_queue;

template <typename... Args>
void logger::log_to_stdout(const level &lvl,
                           const std::format_string<Args...> &fmt, Args... args)
{
    if (logger::get_level() > lvl)
        return;
    std::string formatted_string = std::format(fmt, args...);
    std::cout << std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void logger::log_to_file(const level &lvl,
                         const std::format_string<Args...> &fmt, Args... args)
{
    if (logger::get_level() > lvl && !logger::log_file.is_open())
        return;
    std::string formatted_string = std::format(fmt, args...);
    logger::log_file << std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void logger::log(const level &lvl, const std::format_string<Args...> &fmt,
                 Args... args)
{
    if (logger::get_level() > lvl)
        return;
    log_to_stdout(lvl, fmt, args...);
    if (logger::log_file.is_open())
        log_to_file(lvl, fmt, args...);
}

level constexpr logger::get_level()
{
    return logger::log_level;
}

bool constexpr logger::is_open()
{
    return logger::log_file.is_open();
}

void constexpr logger::set_level(const oak::level &lvl)
{
    logger::log_level = lvl;
}

void constexpr logger::set_log_file(const std::string &file)
{
    logger::log_file.open(file, std::ios::app);
    if (!logger::log_file.is_open())
    {
        std::cout << "Error: Could not open log file" << std::endl;
    }

    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    log_file << "----------" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S")
             << "----------" << std::endl;
}

void constexpr logger::close_file()
{
    if (logger::is_open())
        logger::log_file.close();
}

template <typename... Args>
void out(const std::format_string<Args...> &fmt, Args... args)
{
    logger::log(oak::level::output, fmt, args...);
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
