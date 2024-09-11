# oak - your favourite logger

This is a simple, header-only, C++23 logger with no external dependencies.
This code was originally forked from the logger of [Brenta Engine](https://github.com/San7o/Brenta-Engine)
in order to develop it indipendently from the engine.

```c++
oak::info("hello {}!", name);
```

It features:

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
