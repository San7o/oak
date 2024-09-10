#include "oak.hpp"
#include "test.hpp"

#include <iostream>
#include <sstream>
#include <string>

int errors = 0;
int num_assertions = 0;

int main()
{
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
