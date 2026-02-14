// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

//
// oak2
// ====
//
// Modern rewrite of oak for high performance logging
//

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

namespace oak
{

//
// Macros
//

#define OAK_LOG(level, ...) oak::log(level, __FILE__, __LINE__, __VA_ARGS__)
#define OAK_DEBUG(...)      OAK_LOG(oak::Level::Debug, __VA_ARGS__)
#define OAK_INFO(...)       OAK_LOG(oak::Level::Info, __VA_ARGS__)
#define OAK_WARN(...)       OAK_LOG(oak::Level::Warn, __VA_ARGS__)
#define OAK_ERROR(...)      OAK_LOG(oak::Level::Error, __VA_ARGS__)

#define OAK_LOG2(logger, level, ...) logger->log2(level, __FILE__, __LINE__, __VA_ARGS__)
#define OAK_DEBUG2(logger, ...) OAK_LOG2(logger, oak::Level::Debug, __VA_ARGS__)
#define OAK_INFO2(logger, ...)  OAK_LOG2(logger, oak::Level::Info, __VA_ARGS__)
#define OAK_WARN2(logger, ...)  OAK_LOG2(logger, oak::Level::Warn, __VA_ARGS__)
#define OAK_ERROR2(logger, ...) OAK_LOG2(logger, oak::Level::Error, __VA_ARGS__)

  
enum class Level
{
  Debug     = 0,
  Info      = 1,
  Warn      = 2,
  Error     = 3,
  Disabled  = 4,
  Default   = Info,
};

const char* level_to_string(enum Level level);
  
enum Flags
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

struct Event
{
  unsigned int id;
  std::string name;
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

  FileWriter(const std::filesystem::path &path);

  void write(const std::string& str) override;

private:

  std::ofstream            out;

};

class StdoutWriter : public Writer
{
public:
    
  StdoutWriter() = default;
  
  void write(const std::string& str) override;
    
};

// TODO: NetWriter, UnixWriter

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
  inline void debug(const char *fmt, Args &&...args)
  { log2(Level::Debug, "unknown", 0, fmt, args...); }
  template<typename... Args>
  inline void debug2(const char* file, int line,
                     const char *fmt, Args &&...args)
  { log2(Level::Debug, file, line, fmt, args...); }
  
  template<typename... Args>
  inline void info(const char *fmt, Args &&...args)
  { log2(Level::Info, "unknown", 0, fmt, args...);  }
  template<typename... Args>
  inline void info2(const char* file, int line,
                    const char *fmt, Args &&...args)
  { log2(Level::Info, file, line, fmt, args...);  }
  
  template<typename... Args>
  inline void warn(const char *fmt, Args &&...args)
  { log2(Level::Warn, "unknown", 0, fmt, args...);  }
  template<typename... Args>
  inline void warn2(const char* file, int line,
                    const char *fmt, Args &&...args)
  { log2(Level::Warn, file, line, fmt, args...);  }
  
  template<typename... Args>
  inline void error(const char *fmt, Args &&...args)
  { log2(Level::Error, "unknown", 0, fmt, args...); }
  template<typename... Args>
  inline void error2(const char* file, int line,
                     const char *fmt, Args &&...args)
  { log2(Level::Error, file, line, fmt, args...); }

  template<typename... Args>
  void log(enum Level l, const char *fmt, Args &&...args)
  { log2(l, "unknown", 0, fmt, args...); }
  template<typename... Args>
  void log2(enum Level level, const char* file, int line,
            const char *fmt, Args &&...args);

  // TODO: Add event
  // TODO: Remove event
  // TODO: Add writer
  // TODO: Set / get level
  // TODO: Set / get flags


  // TODO
  template<typename... Args>
  void event(unsigned int id, const char *fmt, Args &&...args)
  {
    // TODO: check that event is enabled (should be atomic / mutexed)
    
    std::string str = std::vformat(fmt, std::make_format_args(args...));
    str = "[EVENT_NAME_TODO] " + str; // TODO

    // TODO: should call all callbacks
    //write(1, str.c_str(), str.size());
  }

private:

  static inline const Formatter default_formatter =
    [](enum Level level, int flags,
       const char* file,
       int line,
       const std::string& log)
  {
    // TODO
    auto level_str = level_to_string(level);
    return "[ " + std::string(level_str) + " ] " + log + "\n";
  };
  
  enum Level          level = Level::Info;
  unsigned long int   flags = Flags::Default;
  std::vector<Event>  events;

  Formatter           formatter = default_formatter;
  std::vector<std::shared_ptr<Writer>> writers;
  
};

//
// Global logger
//

Logger _oak_global_logger;
  
template<typename... Args>
static inline void log(enum oak::Level level, const char* file, int line, const char* fmt, Args &&...args)
{
  oak::_oak_global_logger.log2(level, file, line, fmt, args...);
}
  
} // namespace oak
