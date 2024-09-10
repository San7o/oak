#pragma once

#define ASSERT(x) \
        num_assertions++; \
        if (!(x)) { \
                std::cerr << "Line " << __LINE__ << " in file " \
                << __FILE__ << ": Assertion failed: " << #x << std::endl; \
                errors++;\
        }

#define ASSERT_EQ(x, y) \
        num_assertions++; \
        if ((x) != (y)) { \
                std::cerr << "Line " << __LINE__ << " in file " \
                << __FILE__ << ": Assertion failed: " << x \
                << " != " << y << std::endl; \
                errors++;\
        }

