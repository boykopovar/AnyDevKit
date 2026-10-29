#include "Checks.hpp"
#include <version>
#if !defined(_LIBCPP_VERSION) || _LIBCPP_HAS_LOCALIZATION
#include <sstream>
#include <iomanip>
#include <string>
#endif

void CheckStreams() {
#if defined(_LIBCPP_VERSION) && !_LIBCPP_HAS_LOCALIZATION
    throw std::runtime_error("libc++ localization is disabled; required component unavailable");
#else
    std::stringstream stream;
    stream << std::hex << 255 << ' ' << std::quoted("two words");
    int value = 0; std::string text; stream >> std::hex >> value >> std::quoted(text);
    Require(value == 255 && text == "two words", "Stream formatting and parsing");
    stream.clear(); stream.str("invalid"); stream.exceptions(std::ios::failbit);
    ExpectException<std::ios_base::failure>([&] { stream >> value; });
#endif
}
