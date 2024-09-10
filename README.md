# oak - your favourite logger

This is a simple, header-only, C++23 logger with no external dependencies.
This code was originally forked from the logger of [Brenta Engine](https://github.com/San7o/Brenta-Engine)
in order to develop it indipendently from the engine.

It features:

- [x] multiple logging levels

- [x] log to file

- [ ] log verbose level

- [ ] json output

- [ ] yaml settings

- [ ] async logging and multithreading

- [ ] fuzzing

## Supported log levels

- `OAK_DEBUG`

- `OAK_INFO`

- `OAK_WARNING`

- `OAK_ERROR`

- `OAK_OUTPUT`

- `OAK_DISABLED`

## TODO

- Print through `std::formatter`
- Better api
