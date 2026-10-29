#include "Checks.hpp"
#include <array>
#include <cstdio>
#include <exception>

int main() {
    struct Check { const char* Name; void (*Run)(); };
    const auto checks = std::to_array<Check>({
        {"containers", CheckContainers}, {"memory", CheckMemory},
        {"algorithms", CheckAlgorithms}, {"utilities", CheckUtilities},
        {"strings", CheckStrings}, {"numeric", CheckNumeric},
        {"exceptions", CheckExceptions}, {"coroutines", CheckCoroutines},
        {"atomics", CheckAtomics}, {"locale", CheckLocale},
        {"streams", CheckStreams}, {"regex", CheckRegex},
        {"formatting", CheckFormatting}, {"c compatibility", CheckCCompatibility}
    });
    unsigned failures = 0;
    for (const auto& check : checks) {
        std::printf("RUN %s\n", check.Name);
        std::fflush(stdout);
        try {
            check.Run();
            std::printf("PASS %s\n", check.Name);
        } catch (const std::exception& error) {
            ++failures;
            std::printf("FAIL %s: %s\n", check.Name, error.what());
        } catch (...) {
            ++failures;
            std::printf("FAIL %s: nonstandard exception\n", check.Name);
        }
    }
    std::printf("CppStandardLibrary: %zu groups, %u failures\n", checks.size(), failures);
    if (std::fflush(stdout) != 0) return 2;
    return failures == 0 ? 0 : 1;
}
