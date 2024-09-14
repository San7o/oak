#include "oak.hpp"
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
    ASSERT(level == oak::level::warn);
    auto flags = oak::get_flags();
    ASSERT(flags == 1);
    ASSERT(oak::is_file_open() == false);
}

void test_level()
{
    oak::set_level(oak::level::debug);
    ASSERT_EQ(oak::get_level(), oak::level::debug);
    oak::set_level(oak::level::info);
    ASSERT_EQ(oak::get_level(), oak::level::info);
    oak::set_level(oak::level::warn);
    ASSERT_EQ(oak::get_level(), oak::level::warn);
    oak::set_level(oak::level::error);
    ASSERT_EQ(oak::get_level(), oak::level::error);
    oak::set_level(oak::level::output);
    ASSERT_EQ(oak::get_level(), oak::level::output);
    oak::set_level(oak::level::debug);
}

void test_flags()
{
    oak::set_flags(oak::flags::level);
    ASSERT_EQ(oak::get_flags(), 1);
    oak::set_flags(oak::flags::level, oak::flags::date);
    ASSERT_EQ(oak::get_flags(), 3);
    oak::set_flags(oak::flags::level, oak::flags::date, oak::flags::time);
    ASSERT_EQ(oak::get_flags(), 7);
    oak::set_flags(oak::flags::time);
    ASSERT_EQ(oak::get_flags(), 4);
    oak::set_flags(oak::flags::level);
}

void test_settings_file()
{
    auto ret = oak::settings_file("nope");
    ASSERT(!ret.has_value());

    ret = oak::settings_file("tests/test_settings1.oak");
    ASSERT(ret.has_value());

    ASSERT_EQ(oak::get_level(), oak::level::debug);
    ASSERT_EQ(oak::get_flags(), 31);
    ASSERT_EQ(oak::is_file_open(), true);

    ret = oak::settings_file("tests/test_settings2.oak");
    ASSERT(ret.has_value());
    ASSERT_EQ(oak::get_level(), oak::level::info);
    ASSERT_EQ(oak::get_flags(), 2);
    ASSERT_EQ(oak::is_file_open(), true);
}

void test_file()
{
    auto exp = oak::set_file("/home/root/prova");
    ASSERT(!exp.has_value());

    if (std::filesystem::exists("tests/test_out.txt"))
    {
        std::filesystem::remove("tests/test_out.txt");
    }

    // Create the file
    std::ofstream file("tests/test_out.txt");
    file.close();

    exp = oak::set_file("tests/test_out.txt");
    ASSERT(exp.has_value());
    oak::log_to_file(oak::level::info, "hello file");

    // give time to write
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    oak::close_file();
    ASSERT(std::filesystem::exists("tests/test_out.txt"));
    ASSERT(std::filesystem::file_size("tests/test_out.txt") > 0);

    // clean up
    std::filesystem::remove("tests/test_out.txt");
}

void test_log()
{
    oak::set_level(oak::level::debug);
    oak::set_flags(oak::flags::json);
    oak::log(oak::level::info, "no flags");
    oak::set_flags(oak::flags::level);
    oak::log(oak::level::info, "just level");
    oak::set_flags(oak::flags::level, oak::flags::date, oak::flags::time);
    oak::log(oak::level::info, "level, date and time");
}

void test_macros()
{
    oak::set_flags(oak::flags::level);
    OAK_DEBUG("debug {}", "macro");
    OAK_INFO("info {}", "macro");
    OAK_WARN("warn {}", "macro");
    OAK_ERROR("error {}", "macro");
    OAK_OUTPUT("output {}", "macro");
}

void test_async()
{
    oak::async(oak::level::info, "This was async!");
}

#ifdef OAK_USE_SOCKETS
void test_unix_socket_connect_and_send_message()
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    auto ret = oak::set_socket("/tmp/oak-socket");
    ASSERT(ret.has_value());
    ASSERT(ret.value() > 0);

    oak::log(oak::level::info, "hello socket");
}

void test_net_socket_connect_and_send_message()
{
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    auto ret = oak::set_socket("127.0.0.1", 1234);
    ASSERT(ret.has_value());
    ASSERT(ret.value() > 0);

    oak::log(oak::level::info, "hello socket");
}

void test_unix_socket()
{
    oak::set_flags(oak::flags::level);
    oak::set_level(oak::level::info);

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
    oak::set_flags(oak::flags::level);
    oak::set_level(oak::level::info);

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

int main()
{
    oak::init_writer();

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
    test_async();
#ifdef OAK_USE_SOCKETS
#ifdef __unix__
    test_unix_socket();
    test_net_socket();
#endif
#endif

    oak::stop_writer();

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
