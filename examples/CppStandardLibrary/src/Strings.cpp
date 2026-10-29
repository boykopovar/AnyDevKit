#include "Checks.hpp"
#include <array>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>

void CheckStrings() {
    std::string text(4096, 'x'); text.replace(100, 3, "abc");
    auto copied = text; text.clear();
    Require(copied.substr(100, 3) == "abc" && copied.size() == 4096, "String ownership");
    std::string_view view(copied); Require(view.find("abc") == 100, "String view");
    std::u16string utf16 = u"hello"; std::u32string utf32 = U"hello"; std::wstring wide = L"hello";
    Require(utf16.size() == 5 && utf32.size() == 5 && wide.size() == 5, "Character strings");
    std::array<char, 128> buffer{};
    auto encoded = std::to_chars(buffer.data(), buffer.data() + buffer.size(), 123456789);
    int number = 0; auto decoded = std::from_chars(buffer.data(), encoded.ptr, number);
    Require(encoded.ec == std::errc{} && decoded.ec == std::errc{} && number == 123456789, "Integer charconv");
    auto real = std::to_chars(buffer.data(), buffer.data() + buffer.size(), 3.125);
    double value = 0; auto parsed = std::from_chars(buffer.data(), real.ptr, value);
    Require(real.ec == std::errc{} && parsed.ec == std::errc{} && value == 3.125, "Floating charconv");
    constexpr char invalid[] = "x";
    auto bad = std::from_chars(invalid, invalid + 1, number);
    Require(bad.ec == std::errc::invalid_argument, "Invalid charconv");
    Require(std::stoi("42") == 42 && std::to_string(-12) == "-12", "String conversions");
}
