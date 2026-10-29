#include "Checks.hpp"
#include <format>
#include <string>

void CheckFormatting() {
    Require(std::format("{}:{:04x}:{:.2f}", "value", 42, 1.25) == "value:002a:1.25", "Format output");
    std::string output; std::format_to(std::back_inserter(output), "{} {}", 12, 34);
    Require(output == "12 34" && std::formatted_size("{}", 12345) == 5, "Format iterators");
    ExpectException<std::format_error>([] { static_cast<void>(std::vformat("{", std::make_format_args())); });
}
