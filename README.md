# oak - your favourite logger

Oak is a feature-rich, header-only thread-safe C++23 logger with no external dependencies.
This code was originally forked from the logger of [Brenta Engine](https://github.com/San7o/Brenta-Engine)
in order to develop it indipendently from the engine.

## Quick Tour

### How to log
Log something with the level `info`:
```c++
oak::info("hello {}!", name);
```
```bash
# output
[level=info] hello user!
```
Or use macros if you prefer:
```c++
OAK_INFO("hello {}!", name);
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

### Log to file
```c++
oak::set_file("/tmp/my-log");
```

### Log to socket
```c++
# unix sockets
oak::set_socket("/tmp/a-socket");
# net socket, defaults to tcp
oak::set_socket("127.0.0.1", 1234);
# udp net socket
oak::set_socket("127.0.0.1", 5678, protocol_t::udp);
```

### Json serialization
```c++
oak::set_json_serialization(true);
```
```
{ "level": "output", "date": "2024-09-11", "time": "15:35:20", "pid": 30744, "tid": 9992229128130766714, "message": "Hello Mario" }
```

### Settings file
You can save the settings in a file with `key=value,...`, like this:
```
level = debug
flags = level, date, time, pid, tid
json = true
file = tests/log_test.txt
```
And use this settings like so:
```c++
auto r = oak::settings_file("settings.oak");
if (!r.has_value())
    oak::error("Error opening setting file: {}", r.error());
```

## Features

- [x] multiple logging levels

- [x] log to file

- [x] log to unix sockets

- [x] log to net sockets

- [x] log metadata

- [x] settings file

- [x] json serialization

- [x] thread safe

- [ ] log buffering

- [ ] async logging


## Supported log levels

- `debug`

- `info`

- `warning`

- `error`

- `ouput`

- `disabled`

## Additional log info

- `level`

- `date`

- `time`

- `pid`

- `tid`

# TODO

- fuzzer

- more tests

- documentation
