![oak-banner](./docs/oak_banner.jpeg)

Oak is a lightweight logging library written in C++23, designed to
simplify logging in modern C++ applications.

This code was originally forked from the logger of [Brenta
Engine](https://github.com/San7o/Brenta-Engine) in order to develop it
independently from the engine.

## Features

- Thread safe
- Non-blocking publisher-subscriber pattern: each [writer](#writers)
  runs on its own thread and keeps a local queue of the logs that have
  to be written, so the log call does not need to wait for the output
  to be written
- Customization: Oak is designed with a modular architecture
  so you can easily extend its functionalities and implement new ones (see [writers](#writers) and [formatters](#formatters))
- Log levels API
- Event logging API
- Load configuration from a settings file
- Simple API

# Usage

You can simply copy [include/oak/oak.hpp](./include/oak/oak.hpp) and
[src/oak.cpp](./src/oak.cpp) in your imports and sources
respectively. Alternatively, you can add this repository as a git
submodule and register it in cmake as a subdirectory, or fetch it
using [CPM](https://github.com/cpm-cmake/CPM.cmake):

```
CPMAddPackage(
    NAME oak
    GITHUB_REPOSITORY San7o/oak
    GIT_TAG origin/main)
```

There is a single header, `oak.hpp`:

```c++
#include <oak/oak.hpp>
```

You can create a local `Logger` object, its resources will be
automatically cleaned when it goes out of scope.

```c++
auto logger = oak::Logger();
```

There is also a global logger that is accessible thought functions
with the same signature as a local logger, like `oak::log(...)`
instead of `logger.log(...)`.

## Log level API

You have several logging levels of increasing priority: `Debug`,
`Info`, `Warn` and `Error`. You can set which level is enabled,
meaning that only logs that have the same or higher priority will be
considered.

```c++
logger.info("I use arch, btw");
logger.error("You got an error");

// Using the global logger
oak::info("I use arch, btw");
oak::error("You got an error");
```

Example output:

```
INFO  2026-02-15 14:08:35 | I use arch, btw
ERROR 2026-02-15 14:08:35 | You got an error
```

To collect the file and line of where the log was generated, use the
macro:

```
OAK_DEBUG("Now oak known where this was generated")
OAK_DEBUG2(&logger, "With DEBUG2 we can specify the address of a logger");
```

## Event API

Other than log levels, oak provides an event API which lets you
generate events of a certain type (identified by a number) which will
be logged only if event logging for that type is enabled.

Here is an example

```c++
logger.set_flags(Flags::Json, Flags::Time);   // log additional metadata
logger.enable_event(allocation_event_id, "allocation");
logger.disable_event(exit_event_id, "exit");  // all events are disabled by default

// This will be logged
logger.event(allocation_event_id, "I have allocated something right here");
// This will not be logged
logger.event(exit_event_id, "Exiting example");
```

Output:

```c++
{
  "event": "allocation",
  "data": {
    "time": "14:05:29",
    "log": "I have allocated something right here"
  }
}
```

You can imagine how this can be used to trace all memory
allocations/deallocations, or network usage / connections on
demand. Since the output can be very noisy, it can be enabled and
disabled based on what you need to debug.

## Settings

You tune the logger via getter / setters for the various values. The
most important ones are the `level`, which sets up the logger so that
it processes only logs of a certain level or higher, and `flags` which
configure the logger to log additional metadata or with special
formatting (such as json or color).

```c++
logger.set_level(oak::Level::Debug);
logger.set_flags(oak::Flags::File, oak::Flags::Line, oak::Flags::Time);
```

With all flags enabled, the output would look like this:

```c++
{
  "level": "INFO ",
  "date": "2026-02-15",
  "time": "14:14:46",
  "pid": 14080,
  "tid": 139927568701312,
  "file": "/home/santo/projects/oak/tests/oak_tests.cpp",
  "line": 120,
  "log": "Now with json"
}
```

You can also load the settings from a configuration file:

```c++
logger.load_config_file("settings.oak");
```

A config file looks like this:

```c++
level = debug
flags = level, date, time, tid, pid
file = build/some_log.txt
```

## Writers

When you log something, oak will properly format your string and send
it to its writers. Conceptually, the logger is a single producer, and
the writers are multiple consumers of the log data. Each writer runs
on its own thread, and when data is sent to it, it will update a local
queue of logs which will be written to some output. The specific
output depends on the implementation of the writer. By default, oak
provides the `FileWriter` and `StdoutWriter` classes, and only the
`StdoutWriter` is enabled by default.

The logger does not wait for the writer to finish writing since each
writer has its own log queue, which makes the logger really fast.

You can add and remove writers with the `add_writer` and
`remove_writer` api:

```c++
logger.add_writer<oak::FileWriter>("tests/test_out.txt");
logger.remove_writer(FileWriter::name); // the name is used to identify a writer
```

## Formatters

Before sending the log to the writers, the log input needs to be
formatted. For example, the string "Hello, I am {} form {}" needs to
be completed, additional metadata needs to be added based on the
flags, and other customization such as coloring.

The user can specify a formatter function which can be used instead of
the default one, enabling greater customizability and flexilibty.

```c++
logger.set_formatter(my_formatter);
```

# Contributing

Any new contributor is welcome to this project. Please read
[CONTRIBUTING](./docs/CONTRIBUTING.md) for instructions on how to
contribute.

## Testing

The test project uses cmake. To build and run the tests, run:

```c++
cmake -Bbuild
cmake --build build -j 4
./build/tests
```

## Documentation

The project's documentation uses doxygen, to generate the html
documentation locally, please run:

```bash
make docs
```

## Formatting

The library uses `clang-format` for formatting, the rules are saved in
[.clang-format](./.clang-format).

To format the code, run:

```bash
make format
```

## License

The library is licensed under [MIT](./LICENSE) license.
