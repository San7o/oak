// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

#pragma once

#define ASSERT(x)                                                              \
  num_assertions++;                                                            \
  if (!(x))                                                                    \
  {                                                                            \
    std::cerr << __FILE__                                               \
              << ":" << __LINE__ << ": Assertion failed: " << #x << std::endl; \
    errors++;                                                                  \
  }

#define ASSERT_EQ(x, y)                                                        \
  num_assertions++;                                                            \
  if ((x) != (y))                                                              \
  {                                                                            \
    std::cout << __FILE__                                               \
            << ": " << __LINE__ << ": Assertion failed: " << std::format("{}", x) \
            << " != " << std::format("{}", y) << "\n";                  \
    errors++;                                                                  \
  }
