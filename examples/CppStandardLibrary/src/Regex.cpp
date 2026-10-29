#include "Checks.hpp"
#include <version>
#if !defined(_LIBCPP_VERSION) || _LIBCPP_HAS_LOCALIZATION
#include <regex>
#include <string>
#endif

void CheckRegex() {
#if defined(_LIBCPP_VERSION) && !_LIBCPP_HAS_LOCALIZATION
    throw std::runtime_error("libc++ localization is disabled; required component unavailable");
#else
    const std::regex pattern("([a-z]+)=([0-9]+)");
    const std::string text = "value=123"; std::smatch match;
    Require(std::regex_match(text, match, pattern) && match[1] == "value" && match[2] == "123", "Regex captures");
    Require(std::regex_replace(text, pattern, "$2:$1") == "123:value", "Regex replacement");
    ExpectException<std::regex_error>([] { std::regex invalid("["); });
#endif
}
