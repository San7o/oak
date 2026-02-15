![oak-banner](./docs/oak_banner.jpeg)

Oak is a lightweight logging library written in C++23, designed to
simplify logging in modern C++ applications.

This code was originally forked from the logger of [Brenta
Engine](https://github.com/San7o/Brenta-Engine) in order to develop it
independently from the engine.

## Features

- Thread-Safety
- Simplicity and Ease of Use
- Customization: Oak is designed with a modular architecture so you
  can easily implement new writers and formatters.
- Support for multiple logging levels, event logging, json formatting...
- Load a settings file

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

## Quick Tour

To learn about all the functionalities, please visit the [html
documentation](https://san7o.github.io/oak/). Here is presented a
quick guide to showcase the library's api.

There is a single header, `oak.hpp`:

```c++
#include <oak/oak.hpp>
```

You can create a local `Logger` object, its resources will be
automatically cleaned  when it goes out of scope.

```c++
auto logger = oak::Logger();
```

There is also a global logger that is accessible throught static
functions, like `oak::log(...)` instead of `logger.log(...)`.

## Settings

You tune the logger via getter / setters for the varous values:

```c++
logger.set_level(oak::Level::Debug);
logger.set_flags(oak::Flags::File, oak::Flags::Line, oak::Flags::Time);
```

You can also load the settings from a configuration file:

```c++
logger.load_config_file("settings.oak");
```

## Formatters

// TODO

## Writers

// TODO

## Logging

// TODO

## Event API

# Contributing

Any new contributor is welcome to this project. Please read
[CONTRIBUTING](./docs/CONTRIBUTING.md) for intructions on how to
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
