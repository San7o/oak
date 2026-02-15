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
  
enum class Level
{
  Debug     = 0,
  Info      = 1,
  Warn      = 2,
  Error     = 3,
  Disabled  = 4,
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

  FileWriter(const std::filesystem::path &path);

  void write(const std::string& str) override;
  std::string get_name() override;

private:

  std::ofstream            out;

};

class StdoutWriter : public Writer
{
public:
    
  StdoutWriter() = default;
  
  void write(const std::string& str) override;
  std::string get_name() override;
    
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
            const char *fmt, Args &&...args)
  {
    if (level < this->level) return;
    
    std::string formatted = std::vformat(fmt, std::make_format_args(args...));

    std::string output = this->formatter(level, flags, file, line, formatted);
    
    for (auto& writer : writers)
    {
      writer->write(output);
    }
  }

  // TODO: Add event
  // TODO: Remove event

  template<typename W, typename ...Args>
  void add_writer(Args &&...init_args)
  {
    this->writers.push_back(std::make_shared<W>(init_args...));
    OAK_INFO2(this, "[OAK] Added writer {}", writers.front()->get_name());
  }

  // Returns false if the writer was not found
  bool remove_writer(const std::string &name);

  enum Level get_level() const;
  void  set_level(enum Level level);

  unsigned int get_flags() const;
  template<typename ...F>
  void set_flags(F&&... flags)
  {
    this->flags = 0;
    add_flags(flags...);
  }
  template<typename ...F>
  void add_flags(Flags flag, F&&... flags)
  {
    this->flags = this->flags | (unsigned int) flag;
    add_flags(flags...);
  }
  // Base case
  inline void add_flags(Flags flag)
  { this->flags = this->flags | (unsigned int) flag; };

  std::expected<int, std::string>
  load_config_file(const std::filesystem::path& file);

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

  static std::string colorize(enum Level level, const std::string &str);

  static inline const Formatter default_formatter =
    [](enum Level level, int flags,
       const char* file,
       int line,
       const std::string& log)
  {
    // It ain't pretty, but it does the job

    std::string output;

    bool json = false;
    bool do_color = false;
    if (flags & (unsigned int) Flags::Json)
      json = true;
    if (flags & (unsigned int) Flags::Color)
      do_color = true;

    if (json) output += "{ ";
    
    if (flags & (unsigned int) Flags::Level)
    {
      if (json)
        output += "\"level\": \"" + level_to_string(level) + "\", ";
      else
        output += "[ " + level_to_string(level) + " ] ";
    }
    if (flags & (unsigned int) Flags::Date)
    {
      auto now = std::chrono::system_clock::now();
      auto now_time_t = std::chrono::system_clock::to_time_t(now);
      std::tm now_tm = *std::localtime(&now_time_t);
      std::ostringstream oss;
      if (json)
        oss << "\"date\": \"" << std::put_time(&now_tm, "%Y-%m-%d") << "\", ";
      else
        oss << "[ " << std::put_time(&now_tm, "%Y-%m-%d") << " ] ";
      output += oss.str();
    }
    if (flags & (unsigned int) Flags::Time)
    {
      auto now = std::chrono::system_clock::now();
      auto now_time_t = std::chrono::system_clock::to_time_t(now);
      std::tm now_tm = *std::localtime(&now_time_t);
      std::ostringstream oss;
      if (json)
        oss << "\"time\": \"" << std::put_time(&now_tm, "%H:%M:%S") << "\", ";
      else
        oss << "[ " << std::put_time(&now_tm, "%H:%M:%S") << " ] ";
      output += oss.str();
    }
    if (flags & (unsigned int) Flags::Pid)
    {
      std::string pid = std::to_string(getpid());
      if (json) output += "\"pid\": " + pid + ", ";
      else output += "[ " + pid + " ] ";
    }
    if (flags & (unsigned int) Flags::Tid)
    {
      std::ostringstream oss;
      oss << std::this_thread::get_id();
      std::string tid = oss.str();
      if (json)  output += "\"tid\": " + tid + ", ";
      else output += "[ " + tid + " ] ";
    }
    if (flags & (unsigned int) Flags::File)
    {
      if (json) output += "\"file\": \"" + std::string(file) + "\", ";
      else output += "[ " + std::string(file) + " ] ";
    }

    if (flags & (unsigned int) Flags::Line)
    {
      if (json) output += "\"line\": " + std::to_string(line) + ", ";
      else output += "[ " + std::to_string(line) + " ] ";
    }

    if (json) output += "\"log\": \"";
    
    output += log;
    
    if (json) output += "\" }";
    output += '\n';
    
    if (do_color)
      return colorize(level, output);
    return output;
  };
  
  enum Level          level = Level::Info;
  unsigned long int   flags = (int)Flags::Default;
  std::vector<Event>  events;

  Formatter           formatter = default_formatter;
  std::vector<std::shared_ptr<Writer>> writers;  
};

//
// Global logger
//

Logger& get_global();
  
template<typename... Args>
static inline void log2(enum oak::Level level, const char* file, int line, const char* fmt, Args &&...args)
{ oak::get_global().log2(level, file, line, fmt, args...); }

template<typename... Args>
static inline void log(enum oak::Level level, const char* fmt, Args &&...args)
{ oak::get_global().log(level, fmt, args...); }

template<typename W, typename ...Args>
static inline void add_writer(Args &&...init_args)
{ oak::get_global().add_writer<W>(init_args...); }

static inline bool remove_writer(const std::string &name)
{ return oak::get_global().remove_writer(name); }

static inline enum Level get_level()
{ return oak::get_global().get_level(); }
static inline void  set_level(enum Level level)
{ oak::get_global().set_level(level); }

static inline unsigned int get_flags()
{ return oak::get_global().get_flags(); }
template<typename ...F>
void set_flags(F&&...flags)
{ oak::get_global().set_flags(flags...); }
template<typename ...F>
void add_flags(F&&...flags)
{ oak::get_global().add_flags(flags...); }
std::expected<int, std::string>
load_config_file(const std::filesystem::path& file)
{ return oak::get_global().load_config_file(file); }
  
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
