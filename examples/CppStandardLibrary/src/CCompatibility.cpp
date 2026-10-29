#include "Checks.hpp"
#include <array>
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
    const auto rounding = std::fegetround();
    Require(rounding != -1, "Floating environment");
}
