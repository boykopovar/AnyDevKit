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
    Require(std::locale("C") == locale, "Named C locale");
    ExpectException<std::runtime_error>([] { std::locale invalid("anydevkit-nonexistent-locale"); });
    struct Punctuation : std::numpunct<char> {
        char do_decimal_point() const override { return ','; }
        char do_thousands_sep() const override { return '_'; }
        std::string do_grouping() const override { return "\3"; }
    };
    const std::locale customized(locale, new Punctuation);
    Require(std::use_facet<std::numpunct<char>>(customized).decimal_point() == ',', "Custom locale facet");
    Require(std::use_facet<std::ctype<wchar_t>>(locale).toupper(L'a') == L'A', "Wide locale classification");
    const auto copy = locale; Require(copy == locale && copy.name() == "C", "Locale ownership");
#endif
}
