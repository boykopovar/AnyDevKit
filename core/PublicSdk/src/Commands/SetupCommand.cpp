#include "Commands/SetupCommand.hpp"
#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>

extern char** environ;
#endif
namespace AnyDevKit::PublicSdk {

namespace {

#ifdef _WIN32
std::wstring quote(const std::wstring& argument) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;

    for (const auto character : argument) {
        if (character == L'\\') { ++slashes; continue; }
        result.append(slashes * (character == L'"' ? 2 : 1), L'\\');
        if (character == L'"') result += L'\\';
        result += character;
        slashes = 0;
    }

    result.append(slashes * 2, L'\\');
    result += L'"';
    return result;
}

#endif
int run(const std::filesystem::path& executable, const std::vector<std::string>& arguments) {
#ifdef _WIN32
    auto command = quote(executable.wstring());

    for (const auto& argument : arguments) command += L" " + quote(std::filesystem::path(argument).wstring());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &process)) throw std::system_error(GetLastError(), std::system_category(), "Cannot launch Python");
    CloseHandle(process.hThread);
    const auto wait = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD status = 0;
    const auto success = wait == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &status);
    const auto error = GetLastError();
    CloseHandle(process.hProcess);
    if (!success) throw std::system_error(error, std::system_category(), "Cannot wait for Python");
    return static_cast<int>(status);
#else
    std::string name = executable.string();
    std::vector<char*> values{name.data()};

    for (const auto& argument : arguments) values.push_back(const_cast<char*>(argument.c_str()));
    values.push_back(nullptr);
    pid_t process = 0;
    const auto error = posix_spawn(&process, name.c_str(), nullptr, nullptr, values.data(), environ);
    if (error) throw std::system_error(error, std::generic_category(), "Cannot launch Python");
    int status = 0;

    while (waitpid(process, &status, 0) == -1) if (errno != EINTR) throw std::system_error(errno, std::generic_category(), "Cannot wait for Python");
    if (!WIFEXITED(status)) throw std::runtime_error("Python terminated abnormally");
    return WEXITSTATUS(status);
#endif
}

bool succeeds(const std::filesystem::path& executable, const std::vector<std::string>& arguments) {
    try { return run(executable, arguments) == 0; }
    catch (const std::system_error&) { return false; }
}

}

void SetupCommand::Run(std::span<const std::string> arguments, const std::filesystem::path& sdkRoot) {
    const auto script = sdkRoot / "tools/setup.py";
    if (!std::filesystem::is_regular_file(script)) throw std::runtime_error("Missing SDK tools/setup.py");
    const auto* environment = std::getenv("PATH");
    if (!environment) throw std::runtime_error("PATH is not set; run tools/setup.py with Python 3.11+");
#ifdef _WIN32
    constexpr char Separator = ';';
    const std::vector<std::string> names{"python.exe", "python3.exe", "py.exe"};
#else
    constexpr char Separator = ':';
    const std::vector<std::string> names{"python3", "python", "python3.15", "python3.14", "python3.13", "python3.12", "python3.11"};
#endif
    const std::string paths(environment);

    for (const auto& name : names) {
        std::size_t start = 0;
        while (start <= paths.size()) {
            const auto end = paths.find(Separator, start);
            const auto executable = std::filesystem::absolute(std::filesystem::path(paths.substr(start, end - start)) / name);
            std::error_code error;
            if (std::filesystem::is_regular_file(executable, error)) {
                std::vector<std::string> prefix;
                auto probe = prefix;
                probe.insert(probe.end(), {"-c", "import sys; sys.exit(0 if sys.version_info >= (3, 11) else 1)"});
                if (succeeds(executable, probe)) {
                    prefix.push_back(script.string());
                    prefix.insert(prefix.end(), arguments.begin(), arguments.end());
                    const auto status = run(executable, prefix);
                    if (status != 0) throw std::runtime_error("setup.py exited with code " + std::to_string(status));
                    return;
                }
            }

            if (end == std::string::npos) break;
            start = end + 1;
        }
    }

    throw std::runtime_error("System Python 3.11+ not found; run tools/setup.py with Python 3.11+");
}

}

