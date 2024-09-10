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

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <ostream>
#include <iomanip>

#define OAK_DEBUG(...)                                                             \
    oak::logger::log(oak::log_level::DEBUG, __VA_ARGS__)
#define OAK_INFO(...)                                                              \
    oak::logger::log(oak::log_level::INFO, __VA_ARGS__)
#define OAK_WARNING(...)                                                           \
    oak::logger::log(oak::log_level::WARNING, __VA_ARGS__)
#define OAK_ERROR(...)                                                             \
    oak::logger::log(oak::log_level::ERROR, __VA_ARGS__)
#define OAK_OUTPUT(...)                                                            \
    oak::logger::log(oak::log_level::OUTPUT, __VA_ARGS__)

namespace oak
{

enum log_level
{
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    OUTPUT,
    DISABLED
};

class logger
{
  public:
    static log_level level;
    static std::ofstream log_file;

    static void set_log_level(log_level level);
    static void set_log_file(const std::string &file);
    static void stop();

    template <typename T, typename... Args>
    static void log_to_stdout(T message, Args... args)
    {
        std::cout << message;
        log_to_stdout(args...);
    }

    template <typename T, typename... Args>
    static void log_to_file(T message, Args... args)
    {
        log_file << message;
        log_to_file(args...);
    }
    template <typename... Args>
    static void log(log_level level, Args... args)
    {
        if (logger::level > level)
            return;
        log_to_stdout(level, ": ", args...);
        if (logger::log_file.is_open())
            log_to_file(level, ": ", args...);
    }

  private:
    /* Base cases */
    template <typename T> static void log_to_stdout(T message)
    {
        std::cout << message << std::endl;
    }
    template <typename T> static void log_to_file(T message)
    {
        log_file << message << std::endl;
    }
};

oak::log_level logger::level = oak::log_level::WARNING;
std::ofstream logger::log_file;

void logger::set_log_level(oak::log_level level)
{
    logger::level = level;
}

void logger::stop()
{
    if (logger::log_file.is_open())
        logger::log_file.close();
}

void logger::set_log_file(const std::string &file)
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

template<typename... Args>
void log(Args... args)
{
    logger::log(oak::log_level::OUTPUT, args...);
}

std::ostream &operator<<(std::ostream &os, const log_level level)
{
    switch (level)
    {
    case oak::log_level::DEBUG:
        os << "DEBUG";
        break;
    case oak::log_level::INFO:
        os << "INFO";
        break;
    case oak::log_level::WARNING:
        os << "WARNING";
        break;
    case oak::log_level::ERROR:
        os << "ERROR";
        break;
    case oak::log_level::OUTPUT:
        os << "OUTPUT";
        break;
    case oak::log_level::DISABLED:
        os << "DISABLED";
        break;
    }
    return os;
}

} // namespace oak
