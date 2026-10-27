#include "WindowsStartupBuilder.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("Expected startup object output path");
        AnyDevKit::WindowsStartupBuilder::WriteFile(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
