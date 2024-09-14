#include "fuzztest/fuzztest.h"
#include "oak.hpp"
#include <stdio.h>

void test_log_to_string(const std::string &fmt,
        const std::vector<std::string>& args)
{
    for (const auto &arg : args) {
        oak::log_to_string(oak::level::info, fmt, arg);
    }
}

FUZZ_TEST(fuzz_log_to_string, test_log_to_string);
