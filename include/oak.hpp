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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <print>
#include <string>

#define OAK_DEBUG(...) oak::logger::log(oak::log_level::debug, __VA_ARGS__)
#define OAK_INFO(...) oak::logger::log(oak::log_level::info, __VA_ARGS__)
#define OAK_WARNING(...) oak::logger::log(oak::log_level::warning, __VA_ARGS__)
#define OAK_ERROR(...) oak::logger::log(oak::log_level::error, __VA_ARGS__)
#define OAK_OUTPUT(...) oak::logger::log(oak::log_level::output, __VA_ARGS__)

namespace oak
{

enum class log_level
{
    debug = 0,
    info,
    warning,
    error,
    output,
    disabled,
    __max_log_level
};

class logger
{
  public:
    static log_level level;
    static std::ofstream log_file;

    static void constexpr set_log_level(const log_level &lvl);
    static void constexpr set_log_file(const std::string &file);
    static void constexpr close_file();

    template <typename... Args>
    static void log_to_stdout(const log_level &lvl,
                              const std::format_string<Args...> &fmt,
                              Args... args);
    template <typename... Args>
    static void log_to_file(const log_level &lvl,
                            const std::format_string<Args...> &fmt,
                            Args... args);
    template <typename... Args>
    static void log(const log_level &lvl,
                    const std::format_string<Args...> &fmt, Args... args);
};

oak::log_level logger::level = oak::log_level::warning;
std::ofstream logger::log_file;

template <typename... Args>
void logger::log_to_stdout(const log_level &lvl,
                           const std::format_string<Args...> &fmt, Args... args)
{
    if (logger::level > lvl)
        return;
    std::string formatted_string = std::format(fmt, args...);
    std::cout << std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void logger::log_to_file(const log_level &lvl,
                         const std::format_string<Args...> &fmt, Args... args)
{
    if (logger::level > lvl && !logger::log_file.is_open())
        return;
    std::string formatted_string = std::format(fmt, args...);
    logger::log_file << std::format("{}: {}", lvl, formatted_string);
}

template <typename... Args>
void logger::log(const log_level &lvl, const std::format_string<Args...> &fmt,
                 Args... args)
{
    if (logger::level > lvl)
        return;
    log_to_stdout(lvl, fmt, args...);
    if (logger::log_file.is_open())
        log_to_file(lvl, fmt, args...);
}

void constexpr logger::set_log_level(const oak::log_level &lvl)
{
    logger::level = lvl;
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
    if (logger::log_file.is_open())
        logger::log_file.close();
}

template <typename... Args>
void log(const std::format_string<Args...> &fmt, Args... args)
{
    logger::log(oak::log_level::output, fmt, args...);
}

} // namespace oak

template <> struct std::formatter<oak::log_level>
{
    constexpr auto parse(format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const oak::log_level &level, FormatContext &ctx) const
    {
        switch (level)
        {
        case oak::log_level::error:
            return format_to(ctx.out(), "ERROR");
        case oak::log_level::warning:
            return format_to(ctx.out(), "WARNING");
        case oak::log_level::info:
            return format_to(ctx.out(), "INFO");
        case oak::log_level::debug:
            return format_to(ctx.out(), "DEBUG");
        case oak::log_level::output:
            return format_to(ctx.out(), "OUTPUT");
        default:
            return format_to(ctx.out(), "UNKNOWN");
        }
    }
};
