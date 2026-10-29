#include "Application.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
namespace {
std::filesystem::path executable(const char* name) {
    const std::filesystem::path path(name);
    if (path.has_parent_path()) return std::filesystem::canonical(path);
    const auto* environment = std::getenv("PATH");
    if (!environment) throw std::runtime_error("PATH is not set");
    std::string paths(environment);
#ifdef _WIN32
    constexpr char Separator = ';';
#else
    constexpr char Separator = ':';
#endif
    std::size_t start = 0;
    while (start <= paths.size()) {
        const auto end = paths.find(Separator, start);
        const auto candidate = std::filesystem::path(paths.substr(start, end - start)) / path;
        if (std::filesystem::is_regular_file(candidate)) return std::filesystem::canonical(candidate);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    throw std::runtime_error("Cannot locate adk executable");
}
}

int main(int argc, char** argv) {
    try {
        std::vector<std::string> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        AnyDevKit::PublicSdk::Application::Run(arguments, executable(argv[0]).parent_path().parent_path());
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "adk: " << error.what() << '\n';
        return 1;
    }
}
