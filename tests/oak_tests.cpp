#include "oak/oak.hpp"
#include "test.hpp"

#include <chrono>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int errors = 0;
int num_assertions = 0;

void test_getters()
{
  // default values
  auto level = oak::get_level();
  ASSERT(level == oak::Level::Default);
  auto flags = oak::get_flags();
  ASSERT(flags == (unsigned int) oak::Flags::Level);
}

void test_level()
{
  oak::set_level(oak::Level::Debug);
  ASSERT_EQ(oak::get_level(), oak::Level::Debug);
  oak::set_level(oak::Level::Info);
  ASSERT_EQ(oak::get_level(), oak::Level::Info);
  oak::set_level(oak::Level::Warn);
  ASSERT_EQ(oak::get_level(), oak::Level::Warn);
  oak::set_level(oak::Level::Error);
  ASSERT_EQ(oak::get_level(), oak::Level::Error);
  oak::set_level(oak::Level::Debug);
}

void test_flags()
{
  oak::set_flags(oak::Flags::Level);
  ASSERT_EQ(oak::get_flags(), 1);
  oak::set_flags(oak::Flags::Level, oak::Flags::Date);
  ASSERT_EQ(oak::get_flags(), 3);
  oak::set_flags(oak::Flags::Level, oak::Flags::Date, oak::Flags::Time);
  ASSERT_EQ(oak::get_flags(), 7);
  oak::set_flags(oak::Flags::Time);
  ASSERT_EQ(oak::get_flags(), 4);
  oak::set_flags(oak::Flags::Level);
}

void test_settings_file()
{
  {
    oak::Logger logger = oak::Logger();
    auto ret = logger.load_config_file("nope");
    ASSERT(!ret.has_value());
  }

  {
    oak::Logger logger = oak::Logger();
    auto ret = logger.load_config_file("tests/test_settings1.oak");
    ASSERT(ret.has_value());

    ASSERT_EQ(logger.get_level(), oak::Level::Debug);
    ASSERT_EQ(logger.get_flags(), 31);

    logger.info("Hello log_test!");
  }

  {
    oak::Logger logger = oak::Logger();
    auto ret = logger.load_config_file("tests/test_settings2.oak");
    ASSERT(ret.has_value());
    ASSERT_EQ(logger.get_level(), oak::Level::Info);
    ASSERT_EQ(logger.get_flags(), 2);
  }
}

void test_file()
{
  {
    oak::Logger logger = oak::Logger();
    logger.add_writer<oak::FileWriter>("tests/test_out.txt");
    logger.log(oak::Level::Info, "hello file");

    // give time to write
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    ASSERT(std::filesystem::exists("tests/test_out.txt"));
  }

  ASSERT(std::filesystem::file_size("tests/test_out.txt") > 0);

  // clean up
  std::filesystem::remove("tests/test_out.txt");
}

void test_log()
{
  oak::set_level(oak::Level::Debug);
  oak::set_flags(oak::Flags::Json);
  oak::log(oak::Level::Info, "just json");
  oak::set_flags(oak::Flags::Level);
  oak::log(oak::Level::Info, "just level");
  oak::set_flags(oak::Flags::Level, oak::Flags::Date, oak::Flags::Time);
  oak::log(oak::Level::Info, "level, date and time");
  oak::set_flags(oak::Flags::Level,
                 oak::Flags::Date,
                 oak::Flags::Time,
                 oak::Flags::Pid,
                 oak::Flags::Tid,
                 oak::Flags::File,
                 oak::Flags::Line);
  OAK_INFO("Allllll but no json");
  oak::add_flags(oak::Flags::Json);
  OAK_INFO("Now with json");
}

void test_macros()
{
  oak::set_flags(oak::Flags::Level, oak::Flags::Color);
  OAK_DEBUG("debug {}", "macro");
  OAK_INFO("info {}", "macro");
  OAK_WARN("warn {}", "macro");
  OAK_ERROR("error {}", "macro");
}

/*

#ifdef OAK_USE_SOCKETS
void test_unix_socket_connect_and_send_message()
{
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(200ms);

  auto ret = oak::set_socket("/tmp/oak-socket");
  ASSERT(ret.has_value());
  ASSERT(ret.value() > 0);

  oak::log(oak::Level::info, "hello socket");
}

void test_net_socket_connect_and_send_message()
{
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(200ms);

  auto ret = oak::set_socket("127.0.0.1", 1234);
  ASSERT(ret.has_value());
  ASSERT(ret.value() > 0);

  oak::log(oak::Level::info, "hello socket");
}

void test_unix_socket()
{
  oak::set_flags(oak::Flags::level);
  oak::set_level(oak::Level::info);

  auto ret = oak::set_socket("prova");
  ASSERT(!ret.has_value());
  ASSERT_EQ(ret.error(), "Could not connect to socket");

  // create a socket
  remove("/tmp/oak-socket");
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un sockaddr_un;
  sockaddr_un.sun_family = AF_UNIX;
  strcpy(sockaddr_un.sun_path, "/tmp/oak-socket");
  int r = bind(sock, (struct sockaddr *) &sockaddr_un, sizeof(sockaddr_un));
  ASSERT_EQ(r, 0);

  std::thread t(test_unix_socket_connect_and_send_message);

  r = listen(sock, 5);
  ASSERT_EQ(r, 0);
  int accepted_sock = accept(sock, nullptr, nullptr);
  ASSERT(accepted_sock > 0);
  char buf[1024];
  ssize_t n = read(accepted_sock, buf, 1024);
  ASSERT(n > 0);
  ASSERT_EQ(n, 28);
  ASSERT_EQ(std::string(buf, (unsigned long) n),
            "[ level=info ] hello socket\n");

  t.join();
  oak::close_socket();
}

void test_net_socket()
{
  oak::set_flags(oak::Flags::level);
  oak::set_level(oak::Level::info);

  auto ret = oak::set_socket("127.0.0.1", 1234);
  ASSERT(!ret.has_value());
  ASSERT_EQ(ret.error(), "Could not connect to socket");

  // create a socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in sockaddr_in;
  sockaddr_in.sin_family = AF_INET;
  sockaddr_in.sin_port = htons(1234);
  sockaddr_in.sin_addr.s_addr = INADDR_ANY;
  int r = bind(sock, (struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
  ASSERT_EQ(r, 0);

  std::thread t(test_net_socket_connect_and_send_message);

  r = listen(sock, 5);
  ASSERT_EQ(r, 0);
  int accepted_sock = accept(sock, nullptr, nullptr);
  ASSERT(accepted_sock > 0);
  char buf[1024];
  ssize_t n = read(accepted_sock, buf, 1024);
  ASSERT(n > 0);
  ASSERT_EQ(n, 28);
  ASSERT_EQ(std::string(buf, (unsigned long) n),
            "[ level=info ] hello socket\n");

  t.join();
  oak::close_socket();
}
#endif
*/


void test_event()
{
  oak::Logger logger = oak::Logger();
  logger.set_flags(oak::Flags::Json, oak::Flags::Time);
  logger.activate_event(0, "traces");
  logger.activate_event(1, "entry");
  logger.activate_event(2, "exit");
  logger.activate_event(3, "allocation");

  logger.event(3, "I have allocated something right here");
  logger.event(4, "You should not be able to read this");
}

int main()
{
#ifdef OAK_USE_SOCKETS
  std::cout << "Testing with sockets" << std::endl;
#endif
  std::cout << "\n";

  test_getters();
  test_level();
  test_flags();
  test_settings_file();
  test_file();
  test_log();
  test_macros();
  // test_async();
#ifdef OAK_USE_SOCKETS
#ifdef __unix__
  // test_unix_socket();
  // test_net_socket();
#endif
#endif
  test_event();

  if (errors > 0)
  {
    std::cerr << errors << " tests failed" << std::endl;
    return 1;
  }
  else
  {
    std::cout << num_assertions << " assertions passed" << std::endl;
    return 0;
  }
}
