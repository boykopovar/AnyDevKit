#include "Application.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <vector>
int main(int argc, char** argv) {
    try {
        std::vector<std::string> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        AnyDevKit::PublicSdk::Application::Run(arguments);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "adk: " << error.what() << '\n';
        return 1;
    }
}
