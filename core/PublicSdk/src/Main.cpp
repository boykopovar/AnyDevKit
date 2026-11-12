#include "Application.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#endif
namespace {

std::filesystem::path executable() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(512);
    for (;;) {
        const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) throw std::system_error(GetLastError(), std::system_category(), "Cannot get executable path");
        if (length < buffer.size()) return std::filesystem::path(std::wstring(buffer.data(), length));
        if (buffer.size() >= 32768) throw std::runtime_error("Executable path exceeds Windows path limit");
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    static_cast<void>(_NSGetExecutablePath(nullptr, &size));
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) throw std::runtime_error("Cannot get executable path");
    return std::filesystem::canonical(buffer.data());
#else
    return std::filesystem::read_symlink("/proc/self/exe");
#endif
}

}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--version") { std::cout << "adk " << ANYDEVKIT_VERSION << '\n'; return 0; }
        std::vector<std::string> arguments;

        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        AnyDevKit::PublicSdk::Application::Run(arguments, executable().parent_path().parent_path());
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "adk: " << error.what() << '\n';

        return 1;
    }
}

