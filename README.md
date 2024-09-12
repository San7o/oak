![oak-banner](./docs/oak_banner.jpeg)

Oak is a lightweight and robust logging library for C++23, designed to
simplify logging in modern C++ applications. As a single-header library,
it requires no additional dependencies—simply include it in your project
and start logging immediately.

Built with modern C++ practices, Oak leverages advanced language
features to offer high performance and flexibility. Key features include:
- **Thread-Safety**: Oak ensures safe logging in multi-threaded environments,
                 preventing data races and synchronization issues.
- **Minimal Overhead**: Optimized for performance, Oak introduces minimal
                 runtime overhead, ensuring it doesn't compromise the speed
                 of your application.
- **Simplicity and Ease of Use**: Oak's API is intuitive, allowing you to integrate
                 it seamlessly into your project without a steep learning curve.
- **Customization**: Oak offers a range of customization options, allowing you to
                 tailor the logging experience to your specific requirements.

This code was originally forked from the logger of [Brenta Engine](https://github.com/San7o/Brenta-Engine)
in order to develop it indipendently from the engine.

## Features

- **multiple logging levels**

- **log to file**

- **log to unix sockets**

- **log to net sockets**

- **log metadata**

- **settings file**

- **json serialization**

- **thread safety**

- **log buffering**

- **async logging**

# Quick Tour

To learn about all the functionalities, please visit the [html documentation](https://san7o.github.io/brenta-engine-documentation/oak/v1.0/). Here a quick guide will be made to
showcase the library's api.

### The writer
The logger uses a writer to read the message queue and correctly
writes the output in the specified location, allowing buffering.
```c++
oak::init_writer();
// Do stuff and have fun here
oak::stop_writer();
```

### How to log
Log something with the level `info`:
```c++
oak::info("i love {}!", what);
```
```bash
# output
[level=info] i love oak!
```
Or use macros if you prefer:
```c++
OAK_INFO("add a {} to this library!", star);
```

### Set the Log level
Only logs with an higher level will be logged:
```c++
oak::set_level(oak::level::debug);
```

### Add metadata
```c++
oak::set_flags(oak::flags::level, oak::flags::date);
```
```bash
# example output
[level=info,date=2024-09-11] nice
```

You can also serialize the log adding the flag `oak::flags::json`:
```
{ "level": "output", "date": "2024-09-11", "time": "15:35:20", "pid": 30744, "tid": 9992229128130766714, "message": "Hello Mario" }
```

### Log to file
```c++
auto file = oak::set_file("/tmp/my-log");
if (!file.has_value())
    oak::error("Error opening setting file: {}", file.error());
```
The library uses `std::expected` to handle errors.

### Log to socket
```c++
// unix sockets
oak::set_socket("/tmp/a-socket");
// net socket, defaults to tcp
oak::set_socket("127.0.0.1", 1337);
// udp net socket
oak::set_socket("127.0.0.1", 5678, protocol_t::udp);
```

### Settings file
You can save the settings in a file with `key=value,...`, like this:
```
level = debug
flags = level, date, time, pid, tid
file = tests/log_test.txt
```
And use this settings like so:
```c++
auto r = oak::settings_file("settings.oak");
if (!r.has_value())
    oak::error("Error opening setting file: {}", r.error());
```

### Async logging
```c++
oak::async(oak::level:debug, "Time travelling");
```

# Contributing
Any new contributor is welcome to this project. Please
read [CONTRIBUTING](./CONTRIBUTING.md) for intructions
on how to contribute.

## Testing

The test project uses cmake. To build and run the tests, run:
```c++
cmake -Bbuild
cmake --build build -j 4
./build/tests
```

## Documentation

The project's documentation uses doxygen, to generate the html documentation locally, please run:
```bash
doxygen doxygen.conf
```

## Formatting

The library uses `clang-format` for formatting, the rules
are saved in [.clang-format](./.clang-format). To format
the code, run:
```bash
make format
```

## License

The library is licensed under [MIT](./LICENSE) license.
