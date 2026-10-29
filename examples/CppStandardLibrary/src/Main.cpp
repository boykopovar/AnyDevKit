#include "Checks.hpp"
#include <array>
#include <cstdio>
#include <exception>
#include <cstring>
#include <functional>

int main(int argc, char** argv) {
    if (argc != 1 && (argc != 3 || std::strcmp(argv[1], "--report") != 0)) return 2;
    auto* report = argc == 3 ? std::fopen(argv[2], "w") : stdout;
    if (report == nullptr) return 2;
    struct Check { const char* Name; std::function<void()> Run; };
    const auto checks = std::to_array<Check>({
        {"containers", CheckContainers}, {"memory", CheckMemory},
        {"algorithms", CheckAlgorithms}, {"utilities", CheckUtilities},
        {"strings", CheckStrings}, {"numeric", CheckNumeric},
        {"exceptions", CheckExceptions}, {"coroutines", CheckCoroutines},
        {"atomics", CheckAtomics}, {"locale", CheckLocale},
        {"streams", CheckStreams}, {"file streams", [&] { CheckFileStreams(argc == 3 ? argv[2] : nullptr); }}, {"regex", CheckRegex},
        {"formatting", CheckFormatting}, {"c compatibility", CheckCCompatibility}
    });
    unsigned failures = 0;
    for (const auto& check : checks) {
        std::fprintf(report, "RUN %s\n", check.Name);
        if (std::fflush(report) != 0) return 2;
        try {
            check.Run();
            std::fprintf(report, "PASS %s\n", check.Name);
        } catch (const std::exception& error) {
            ++failures;
            std::fprintf(report, "FAIL %s: %s\n", check.Name, error.what());
        } catch (...) {
            ++failures;
            std::fprintf(report, "FAIL %s: nonstandard exception\n", check.Name);
        }
    }
    std::fprintf(report, "CppStandardLibrary: %zu groups, %u failures\n", checks.size(), failures);
    if (std::fflush(report) != 0) return 2;
    if (argc == 3 && std::fclose(report) != 0) return 2;
    return failures == 0 ? 0 : 1;
}
