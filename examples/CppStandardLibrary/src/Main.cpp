#include "Checks.hpp"
#include "Screen.hpp"
#include "CalculatorError.hpp"
#include <memory>
#include <array>
#include <cstdio>
#include <exception>
#include <cstring>
#include <functional>

namespace {

int run(std::FILE* report, const char* reportPath, Screen* screen) {
    struct Check { const char* Name; std::function<void()> Run; };
    const auto checks = std::to_array<Check>({
        {"containers", CheckContainers}, {"memory", CheckMemory},
        {"algorithms", CheckAlgorithms}, {"utilities", CheckUtilities},
        {"strings", CheckStrings}, {"numeric", CheckNumeric},
        {"exceptions", CheckExceptions}, {"coroutines", CheckCoroutines},
        {"atomics", CheckAtomics}, {"locale", CheckLocale},
        {"streams", CheckStreams}, {"file streams", [&] { CheckFileStreams(reportPath); }}, {"regex", CheckRegex},
        {"formatting", CheckFormatting}, {"c compatibility", CheckCCompatibility}
    });
    unsigned failures = 0;
    for (std::size_t index = 0; index < checks.size(); ++index) {
        const auto& check = checks[index];
        if (screen) screen->Begin(index, check.Name);
        std::fprintf(report, "RUN %s\n", check.Name);
        if (std::fflush(report) != 0) throw std::runtime_error("Cannot flush report");
        try {
            check.Run();
            if (screen) screen->Finish(index, true, {});
            std::fprintf(report, "PASS %s\n", check.Name);
        } catch (const std::exception& error) {
            ++failures;
            if (screen) screen->Finish(index, false, error.what());
            std::fprintf(report, "FAIL %s: %s\n", check.Name, error.what());
        } catch (...) {
            ++failures;
            if (screen) screen->Finish(index, false, "Nonstandard exception");
            std::fprintf(report, "FAIL %s: nonstandard exception\n", check.Name);
        }
    }
    std::fprintf(report, "CppStandardLibrary: %zu groups, %u failures\n", checks.size(), failures);
    if (std::fflush(report) != 0) throw std::runtime_error("Cannot flush report");

    if (screen) screen->Complete(failures);
    return failures == 0 ? 0 : 1;
}

}

int main(int argc, char** argv) {
    bool headless = ANYDEVKIT_CPP_HEADLESS;
#ifdef _WIN32
    const char* reportPath = "CppStandardLibrary-results.txt";
#else
    const char* reportPath = "/data/CppStandardLibrary-results.txt";
#endif
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--headless") == 0) headless = true;
        else if (std::strcmp(argv[index], "--report") == 0 && index + 1 < argc) reportPath = argv[++index];
        else return 2;
    }
    std::unique_ptr<Screen> screen;
    std::FILE* report = nullptr;
    try {
        if (!headless) screen = std::make_unique<Screen>();
        report = std::fopen(reportPath, "w");
        if (report == nullptr) throw std::runtime_error("Cannot create report file");
        const int result = run(report, reportPath, screen.get());
        const auto closeResult = std::fclose(report);
        report = nullptr;
        if (closeResult != 0) throw std::runtime_error("Cannot close report file");
        if (screen) screen->KeepOpen();
        return result;
    } catch (const std::exception& error) {
        if (report) std::fclose(report);
        if (screen) { screen->Error(error.what()); screen->KeepOpen(); }
        return 2;
    } catch (const CalculatorError& error) {
        if (report) std::fclose(report);
        if (screen) { screen->Error(error.Message); screen->KeepOpen(); }
        return 2;
    }
}
