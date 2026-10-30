#include "CommandLine/Arguments.hpp"
#include <charconv>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
namespace AnyDevKit::PublicSdk {
Arguments::Arguments(std::span<const std::string> arguments) {
    const std::set<std::string> flags{"exec", "only-sce", "case-insensitive", "omit-implicit-primitive-id"};
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const auto& argument = arguments[i];
        if (!argument.starts_with("--") || argument.size() == 2) throw std::invalid_argument("Expected --option: " + argument);
        auto name = argument.substr(2);
        if (flags.contains(name)) {
            if (i + 1 < arguments.size() && (arguments[i + 1] == "true" || arguments[i + 1] == "false")) values[name].push_back(arguments[++i]);
            else values[name].emplace_back("true");
        }
        else {
            if (i + 1 == arguments.size() || arguments[i + 1].starts_with("--")) throw std::invalid_argument("Missing value for --" + name);
            values[name].push_back(arguments[++i]);
        }
    }
}
std::vector<std::string> Arguments::Many(const std::string& name) {
    auto entry = values.extract(name);
    return entry.empty() ? std::vector<std::string>{} : std::move(entry.mapped());
}
std::string Arguments::Require(const std::string& name) {
    auto items = Many(name);
    if (items.size() != 1 || items.front().empty()) throw std::invalid_argument("Expected exactly one --" + name);
    return std::move(items.front());
}
std::string Arguments::Optional(const std::string& name, const std::string& defaultValue) {
    if (!values.contains(name)) return defaultValue;
    return Require(name);
}
bool Arguments::Flag(const std::string& name) {
    if (!values.contains(name)) return false;
    const auto value = Require(name);
    if (value != "true" && value != "false") throw std::invalid_argument("Invalid flag --" + name);
    return value == "true";
}
void Arguments::MergeDefaults(const std::map<std::string, std::vector<std::string>>& defaults) {
    for (const auto& [name, items] : defaults) if (!values.contains(name)) values.emplace(name, items);
}
void Arguments::Finish() const {
    if (!values.empty()) throw std::invalid_argument("Unexpected option --" + values.begin()->first);
}
std::uint64_t Arguments::Number(const std::string& value) {
    std::string_view digits(value);
    int base = 10;
    if (digits.starts_with("0x")) { digits.remove_prefix(2); base = 16; }
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(digits.data(), digits.data() + digits.size(), result, base);
    if (digits.empty() || parsed.ec != std::errc{} || parsed.ptr != digits.data() + digits.size()) throw std::invalid_argument("Invalid unsigned integer: " + value);
    return result;
}
std::uint32_t Arguments::Number32(const std::string& value) {
    const auto result = Number(value);
    if (result > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Integer exceeds 32 bits: " + value);
    return static_cast<std::uint32_t>(result);
}
}
