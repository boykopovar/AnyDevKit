#include "Checks.hpp"
#include <array>
#include <climits>
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

void CheckCCompatibility() {
    std::array<char, 64> text{};
    Require(std::snprintf(text.data(), text.size(), "%d", 123) == 3 && std::strcmp(text.data(), "123") == 0, "C formatting");
    char* end = nullptr;
    Require(std::strtol(text.data(), &end, 10) == 123 && *end == 0, "C conversion");
    Require(std::toupper('a') == 'A' && std::wcslen(L"abc") == 3, "C characters");
    auto* memory = static_cast<unsigned char*>(std::malloc(128));
    if (!memory) throw std::bad_alloc();
    std::memset(memory, 42, 128); bool valid = memory[127] == 42; std::free(memory);
    Require(valid, "C allocation");
    std::array<char, 128> formatted{};
    Require(std::snprintf(formatted.data(), formatted.size(), "%ld %lld %.2f %.2Lf", -123L, 45678901234LL, 1.25, 2.5L) > 0 && std::strcmp(formatted.data(), "-123 45678901234 1.25 2.50") == 0, "Mixed variadic ABI");
    Require(std::snprintf(formatted.data(), formatted.size(), "%*.*f", 7, 2, 1.5) == 7 && std::strcmp(formatted.data(), "   1.50") == 0, "Variadic width and precision");
    struct ScanResult { long Value = 0; unsigned Guard = 0x12345678; } scanned;
    double number = 0;
    Require(std::sscanf("-17 2.5", "%ld %lf", &scanned.Value, &number) == 2 && scanned.Value == -17 && scanned.Guard == 0x12345678 && number == 2.5, "Scanning ABI and long width");
    std::array<wchar_t, 64> wide{};
    Require(std::swprintf(wide.data(), wide.size(), L"%ld %.1f", -42L, 1.5) == 7 && std::wcscmp(wide.data(), L"-42 1.5") == 0, "Wide formatting ABI");
    errno = 0;
    Require(std::strtoul("-1", nullptr, 10) == ULONG_MAX && errno == 0, "Unsigned negative conversion");
    const auto rounding = std::fegetround();
    Require(rounding != -1, "Floating environment");
}
