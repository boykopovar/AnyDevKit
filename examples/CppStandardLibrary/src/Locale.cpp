#include "Checks.hpp"
#include <version>
#if !defined(_LIBCPP_VERSION) || _LIBCPP_HAS_LOCALIZATION
#include <locale>
#include <string>
#endif

void CheckLocale() {
#if defined(_LIBCPP_VERSION) && !_LIBCPP_HAS_LOCALIZATION
    throw std::runtime_error("libc++ localization is disabled; required component unavailable");
#else
    const auto locale = std::locale::classic();
    Require(std::use_facet<std::ctype<char>>(locale).toupper('a') == 'A', "Locale classification");
    Require(std::use_facet<std::numpunct<char>>(locale).decimal_point() == '.', "Locale punctuation");
    const auto copy = locale; Require(copy == locale && copy.name() == "C", "Locale ownership");
#endif
}
