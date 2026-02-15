// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

#pragma once

#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <queue>
#include <condition_variable>
#include <filesystem>
#include <print>
#include <memory>
#include <functional>
#include <expected>

namespace oak
{

//
// Macros
//

#define OAK_LOG(level, ...) oak::log2(level, __FILE__, __LINE__, __VA_ARGS__)
#define OAK_DEBUG(...)      OAK_LOG(oak::Level::Debug, __VA_ARGS__)
#define OAK_INFO(...)       OAK_LOG(oak::Level::Info, __VA_ARGS__)
#define OAK_WARN(...)       OAK_LOG(oak::Level::Warn, __VA_ARGS__)
#define OAK_ERROR(...)      OAK_LOG(oak::Level::Error, __VA_ARGS__)

#define OAK_LOG2(logger, level, ...) logger->log2(level, __FILE__, __LINE__, __VA_ARGS__)
#define OAK_DEBUG2(logger, ...) OAK_LOG2(logger, oak::Level::Debug, __VA_ARGS__)
#define OAK_INFO2(logger, ...)  OAK_LOG2(logger, oak::Level::Info, __VA_ARGS__)
#define OAK_WARN2(logger, ...)  OAK_LOG2(logger, oak::Level::Warn, __VA_ARGS__)
#define OAK_ERROR2(logger, ...) OAK_LOG2(logger, oak::Level::Error, __VA_ARGS__)

#define OAK_EVENT(type, ...)    oak::event(type, __VA_ARGS__);
#define OAK_EVENT2(type, logger, ...) logger.event(type, __VA_ARGS__);
  
enum class Level
{
  Debug     = 0,
  Info      = 1,
  Warn      = 2,
  Error     = 3,
  Disabled  = 4,
  Event     = 5,
  Default   = Info,
};

std::string level_to_string(enum Level level);
  
enum class Flags : unsigned int
{
  None    = 0,
  Level   = 1,
  Date    = 1 << 1,
  Time    = 1 << 2,
  Pid     = 1 << 3,
  Tid     = 1 << 4,
  Json    = 1 << 5,
  Color   = 1 << 6,
  File    = 1 << 7,
  Line    = 1 << 8,
  Default = Level,
};

//
// Global logger
//

// Consumes events and logs and outputs them
class Writer
{
public:

  Writer() = default;
  virtual ~Writer();
  
  // Derived classes just have to implement this
  // No locking is required here
  virtual void write(const std::string& str) = 0;
  // A name that identifies the writer
  virtual std::string get_name() = 0;
    
  void submit(const std::string &log);
  void write_loop();
  void stop();

private:

  bool                     should_stop = false;
  std::queue<std::string>  logs;
  // Guards push / pop to the logs queue
  std::mutex               logs_mutex;
  // This notification is activated when new logs arrive, and
  // wakes up the consumer
  std::condition_variable  cv;
    
};

class FileWriter : public Writer
{
public:

  static const std::string name; // set the name as static to make it
                                 // easily accessible
  
  FileWriter(const std::filesystem::path &path);

  void write(const std::string& str) override;
  std::string get_name() override;

private:

  std::ofstream            out;

};

class StdoutWriter : public Writer
{
public:

  static const std::string name;
  
  StdoutWriter() = default;
  
  void write(const std::string& str) override;
  std::string get_name() override;
    
};

// Logger object
class Logger
{
public:

  using Formatter = std::function<std::string(enum Level, int flags,
                                              const char* file,
                                              int line,
                                              const std::string& log)>;
  
  Logger();
  ~Logger() = default;

  // Logging functions

  template<typename... Args>
  inline void debug(const char *fmt, Args &&...args);
  template<typename... Args>
  inline void debug2(const char* file, int line,
                     const char *fmt, Args &&...args);
  
  template<typename... Args>
  inline void info(const char *fmt, Args &&...args);
  template<typename... Args>
  inline void info2(const char* file, int line,
                    const char *fmt, Args &&...args);
  
  template<typename... Args>
  inline void warn(const char *fmt, Args &&...args);
  template<typename... Args>
  inline void warn2(const char* file, int line,
                    const char *fmt, Args &&...args);
  
  template<typename... Args>
  inline void error(const char *fmt, Args &&...args);
  template<typename... Args>
  inline void error2(const char* file, int line,
                     const char *fmt, Args &&...args);

  template<typename... Args>
  void log(enum Level l, const char *fmt, Args &&...args);
  template<typename... Args>
  void log2(enum Level level, const char* file, int line,
            const char *fmt, Args &&...args);

  template<typename W, typename ...Args>
  void add_writer(Args &&...init_args);

  // Returns false if the writer was not found
  bool remove_writer(const std::string &name);

  enum Level get_level() const;
  void  set_level(enum Level level);

  unsigned int get_flags() const;
  template<typename ...F>
  void set_flags(F&&... flags);
  template<typename ...F>
  void add_flags(Flags flag, F&&... flags);
  // Base case
  inline void add_flags(Flags flag);

  std::expected<int, std::string>
  load_config_file(const std::filesystem::path& file);

  void set_formatter(Formatter formatter);
  
  // Event api

  // Only enabled events will be logged
  void enable_event(unsigned int id, const std::string& name);
  void disable_event(unsigned int id);
  
  template<typename... Args>
  void event2(const char* file, int line, unsigned int id,
              const char *fmt, Args &&...args);
  template<typename... Args>
  void event(unsigned int id, const char *fmt, Args &&...args);

private:

  static std::string colorize(enum Level level, const std::string &str);
  static std::string json_formatter(enum Level level, int flags,
                                    const char* file, int line,
                                    const std::string& log);
  
  enum Level          level = Level::Info;
  unsigned long int   flags = (int)Flags::Default;
  std::mutex          logger_mutex;
  std::unordered_map<unsigned int, std::string>  events;

  static const Formatter default_formatter;
  Formatter              formatter = default_formatter;
  std::vector<std::shared_ptr<Writer>> writers;  
};

//
// Global logger
//

Logger& get_global();
  
template<typename... Args>
static inline void log2(enum oak::Level level, const char* file, int line, const char* fmt, Args &&...args);

template<typename... Args>
static inline void log(enum oak::Level level, const char* fmt, Args &&...args);

template<typename W, typename ...Args>
static inline void add_writer(Args &&...init_args);

static inline bool remove_writer(const std::string &name);

static inline enum Level get_level();
static inline void  set_level(enum Level level);

static inline unsigned int get_flags();
template<typename ...F>
static inline void set_flags(F&&...flags);
template<typename ...F>
static inline void add_flags(F&&...flags);
static inline std::expected<int, std::string>
load_config_file(const std::filesystem::path& file);

static inline void enable_event(unsigned int id,
                                const std::string& name);

static inline void disable_event(unsigned int id);

template<typename... Args>
static inline void event(unsigned int id, const char *fmt, Args &&...args);
  
#include "oak.impl"
  
} // namespace oak

template <> struct std::formatter<oak::Level>
{
  constexpr auto parse(format_parse_context &ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const oak::Level &level, FormatContext &ctx) const
  {
    switch (level)
    {
    case oak::Level::Error:
      return format_to(ctx.out(), "error");
    case oak::Level::Warn:
      return format_to(ctx.out(), "warn");
    case oak::Level::Info:
      return format_to(ctx.out(), "info");
    case oak::Level::Debug:
      return format_to(ctx.out(), "debug");
    case oak::Level::Event:
      return format_to(ctx.out(), "event");
    default:
      return format_to(ctx.out(), "unknown");
    }
  }
};

template <> struct std::formatter<oak::Flags>
{
  constexpr auto parse(format_parse_context &ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const oak::Flags &flags, FormatContext &ctx) const
  {
    switch (flags)
    {
    case oak::Flags::None:
      return format_to(ctx.out(), "none");
    case oak::Flags::Level:
      return format_to(ctx.out(), "level");
    case oak::Flags::Date:
      return format_to(ctx.out(), "date");
    case oak::Flags::Time:
      return format_to(ctx.out(), "time");
    case oak::Flags::Tid:
      return format_to(ctx.out(), "pid");
    case oak::Flags::Tid:
      return format_to(ctx.out(), "tid");
    default:
      return format_to(ctx.out(), "unknown");
    }
  }
};
