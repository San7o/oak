# oak - your favourite logger

This is a simple, header-only, C++23 logger with no external dependencies.
This code was originally forked from the logger of [Brenta Engine](https://github.com/San7o/Brenta-Engine)
in order to develop it indipendently from the engine.

## Quick Tour
Set the log level. Only logs with an higher level will be logged:
```c++
oak::set_level(oak::level::debug);
```

Log something with the level `info`:
```c++
oak::info("hello {}!", name);
```
```bash
# output
INFO: hello user!
```

Select additional information to log:
```c++
oak::set_flags(oak::flags::level, oak::flags::date);
```
```bash
# example output
INFO 2024-09-11 11:32:09: nice
```

Log to file:
```c++
oak::set_file("/tmp/my-log");
```

Log to socket:
```c++
# unix sockets
oak::set_socket("/tmp/a-socket");
# net socket, defaults to tcp
oak::set_socket("127.0.0.1", 1234);
# udp net socket
oak::set_socket("127.0.0.1", 5678, protocol_t::udp);
```

## Features

- [x] multiple logging levels

- [x] log to file

- [x] log to unix sockets

- [x] log to net sockets

- [ ] log verbose level

- [ ] json output

- [ ] yaml settings

- [ ] async logging and multithreading

- [ ] fuzzing

## Supported log levels

- `debug`

- `info`

- `warning`

- `error`

- `ouput`

- `disabled`
