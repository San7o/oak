// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

#include <oak/oak.hpp>

using namespace oak;

std::string oak::level_to_string(enum Level level)
{
  switch (level)
  {
  case oak::Level::Debug:    return "DEBUG";
  case oak::Level::Info:     return "INFO";
  case oak::Level::Warn:     return "WARN";
  case oak::Level::Error:    return "ERROR";
  case oak::Level::Disabled: return "DISABLED";
  default:                   return "UNKNOWN";
  }
}


void Writer::submit(const std::string &log)
{
  {
    std::lock_guard<std::mutex> lock(this->logs_mutex);
    this->logs.push(log);
  }
  this->cv.notify_one();  // unlock the consumer
}

Writer::~Writer()
{
  this->stop();
}

void Writer::write_loop()
{          
  while (1)
  {
    std::queue<std::string> local_logs = {};
    {
      std::unique_lock<std::mutex> lock(this->logs_mutex);
        
      this->cv.wait(lock, [this] {
        // When this evaluates to true, this block stops waiting
        // Note that this is checked before waiting in the
        // first place, and is checked every time a notification
        // is sent.
        return !this->logs.empty() || this->should_stop;
      });

      if (this->should_stop && this->logs.empty()) return;
          
      std::swap(local_logs, this->logs);  // instant swap
    } // logs_mutex released
      
    while(!local_logs.empty())
    {
      this->write(local_logs.front());
      local_logs.pop();
    }
  }
}

void Writer::stop()
{
  {
    std::lock_guard<std::mutex> lock(this->logs_mutex);
    this->should_stop = true;
  }
  this->cv.notify_all(); // Wake up the consumer so it sees should_stop is true
  return;
}

FileWriter::FileWriter(const std::filesystem::path &path)
{
  auto file = std::ofstream(path);
  if (!file.is_open())
  {
    std::print("[ERROR] [oak] Error creating writer for file {}", path.c_str());
    return;
  }

  this->out = std::move(file);
}

void FileWriter::write(const std::string& str)
{
  if (this->out.is_open())
    this->out << str;
  return;
}

std::string FileWriter::get_name()
{
  return "file_writer";
}

void StdoutWriter::write(const std::string& str)
{
  std::print("{}", str);
  return;
}

std::string StdoutWriter::get_name()
{
  return "stdout_writer";
}

Logger::Logger()
{
  auto writer = std::make_shared<StdoutWriter>();
  this->writers.push_back(writer);
    
  std::jthread t([writer] { writer->write_loop(); });
  t.detach();

  OAK_INFO2(this, "[ OAK ] Initialized writer {}", writer->get_name());
}

bool Logger::remove_writer(const std::string &name)
{
  std::lock_guard<std::mutex> lock(this->logger_mutex);
  for (auto it = this->writers.begin(); it != this->writers.end(); ++it)
  {
    if ((*it)->get_name() == name)
    {
      this->writers.erase(it);
      OAK_INFO2(this, "Removed writer {}", name);
      return true;
    }
  }
  return false;
}

enum Level Logger::get_level() const
{
  return this->level;
}

void Logger::set_level(enum Level level)
{
  std::lock_guard<std::mutex> lock(this->logger_mutex);
  this->level = level;
  return;
}

void Logger::set_formatter(Formatter formatter)
{
  std::lock_guard<std::mutex> lock(this->logger_mutex);
  this->formatter = formatter;
  return;
}

unsigned int Logger::get_flags() const
{
  return this->flags;
}

// Colors

// Foregound
#define RST "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

// for string literals
#define FRED(x) KRED x RST
#define FGRN(x) KGRN x RST
#define FYEL(x) KYEL x RST
#define FBLU(x) KBLU x RST
#define FMAG(x) KMAG x RST
#define FCYN(x) KCYN x RST
#define FWHT(x) KWHT x RST

#define FRED_S(x) std::string(KRED) + x + std::string(RST)
#define FGRN_S(x) std::string(KGRN) + x + std::string(RST)
#define FYEL_S(x) std::string(KYEL) + x + std::string(RST)
#define FBLU_S(x) std::string(KBLU) + x + std::string(RST)
#define FMAG_S(x) std::string(KMAG) + x + std::string(RST)
#define FCYN_S(x) std::string(KCYN) + x + std::string(RST)
#define FWHT_S(x) std::string(KWHT) + x + std::string(RST)

std::string Logger::colorize(enum Level level, const std::string &str)
{
  switch (level)
  {
  case Level::Debug:
    return FCYN_S(str);
  case Level::Info:
    return FBLU_S(str);
  case Level::Warn:
    return FYEL_S(str);
  case Level::Error:
    return FRED_S(str);
  default:
    return str;
  }
}

Logger& oak::get_global()
{
  static Logger instance;
  return instance;
}

std::expected<int, std::string>
Logger::load_config_file(const std::filesystem::path& file)
{
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
        set_level(Level::Debug);
      else if (value == "info")
        set_level(Level::Info);
      else if (value == "warn")
        set_level(Level::Warn);
      else if (value == "error")
        set_level(Level::Error);
      else
        return std::unexpected("Invalid log level in file");
    }
    else if (key == "flags")
    {
      set_flags(Flags::None);
      while (value.find(',') != std::string::npos)
      {
        std::string flag = value.substr(0, value.find(','));
        value = value.substr(value.find(',') + 1);
        if (flag == "none")
          add_flags(Flags::None);
        else if (flag == "level")
          add_flags(Flags::Level);
        else if (flag == "date")
          add_flags(Flags::Date);
        else if (flag == "time")
          add_flags(Flags::Time);
        else if (flag == "pid")
          add_flags(Flags::Pid);
        else if (flag == "tid")
          add_flags(Flags::Tid);
        else if (flag == "json")
          add_flags(Flags::Json);
        else
          return std::unexpected("Invalid flags in file");
      }
      // get last element
      if (value == "none")
        add_flags(Flags::None);
      else if (value == "level")
        add_flags(Flags::Level);
      else if (value == "date")
        add_flags(Flags::Date);
      else if (value == "time")
        add_flags(Flags::Time);
      else if (value == "pid")
        add_flags(Flags::Pid);
      else if (value == "tid")
        add_flags(Flags::Tid);
      else if (value == "json")
        add_flags(Flags::Json);
      else
        return std::unexpected("Invalid flags in file");
    }
    else if (key == "file")
    {
      add_writer<FileWriter>(value);
    }
    else
    {
      return std::unexpected("Invalid key in file");
    }
  }

  return 0;
}



void Logger::activate_event(unsigned int id, const std::string &name)
{
  std::lock_guard<std::mutex> lock(this->logger_mutex);
  this->events[id] = name;
}

void Logger::deactivate_event(unsigned int id)
{
  std::lock_guard<std::mutex> lock(this->logger_mutex);
  this->events.erase(id);
}
  
