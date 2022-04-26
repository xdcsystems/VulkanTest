#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "Application.h"

int main() {
  try {
    Application app("Vulkan");
    app.run();
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cin.get();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
