// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o

#include <oak/oak.hpp>

int main()
{
  auto logger = oak::Logger();
  auto r = logger.load_config_file("settings.oak");
  if (!r.has_value())
  {
    logger.error("Error opening setting file: {}", r.error());
    return 1;
  }

  std::string name = "Mario";
  logger.info("Hello {}", name);
  return 0;
}
