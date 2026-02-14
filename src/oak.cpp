// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

#include <oak/oak.hpp>

using namespace oak;

const char* oak::level_to_string(enum Level level)
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

void StdoutWriter::write(const std::string& str)
{
  std::print("{}", str);
  return;
}

Logger::Logger()
{
  auto writer = std::make_shared<StdoutWriter>();
  this->writers.push_back(writer);
    
  std::jthread t([writer] { writer->write_loop(); });
  t.detach();

  OAK_INFO2(this, "[ OAK ] Initialized writer");
}

template<typename ...Args>
void Logger::log2(enum Level level, const char* file, int line,
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
