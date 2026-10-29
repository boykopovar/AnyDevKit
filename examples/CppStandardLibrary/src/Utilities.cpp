#include "Checks.hpp"
#include <any>
#include <bit>
#include <bitset>
#include <compare>
#include <concepts>
#include <functional>
#include <limits>
#include <optional>
#include <source_location>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <variant>

void CheckUtilities() {
    static_assert(std::integral<int> && std::is_trivially_copyable_v<unsigned>);
    static_assert(std::numeric_limits<unsigned>::digits >= 16);
    std::any any = std::string(100, 'a'); auto copy = any;
    any.reset(); Require(std::any_cast<const std::string&>(copy).size() == 100, "Any ownership");
    ExpectException<std::bad_any_cast>([&] { static_cast<void>(std::any_cast<int>(copy)); });
    std::variant<int, std::string> variant = 7; variant = std::string(80, 'b');
    Require(std::visit([](const auto& value) -> std::size_t { if constexpr (std::integral<std::decay_t<decltype(value)>>) return value; else return value.size(); }, variant) == 80, "Variant visit");
    ExpectException<std::bad_variant_access>([&] { static_cast<void>(std::get<int>(variant)); });
    std::optional<std::string> optional; ExpectException<std::bad_optional_access>([&] { static_cast<void>(optional.value()); });
    optional.emplace(60, 'c'); Require(optional->size() == 60, "Optional construction");
    std::function<int(int)> function = [text = std::string(128, 'x')](int x) { return x + static_cast<int>(text.size()); };
    Require(function(2) == 130, "Function allocated capture");
    std::function<void()> empty; ExpectException<std::bad_function_call>([&] { empty(); });
    const auto tuple = std::make_tuple(2, 3, 4);
    Require(std::apply([](auto... x) { return (x + ...); }, tuple) == 9, "Tuple apply");
    std::bitset<128> bits; bits.set(100); Require(bits.count() == 1 && bits.test(100), "Bitset");
    Require(std::popcount(0x55u) == 4 && std::rotl(1u, 3) == 8, "Bit operations");
    Require(std::type_index(typeid(int)) != std::type_index(typeid(double)), "Type index");
    Require(std::source_location::current().line() != 0 && (3 <=> 4) < 0, "Language utilities");
}
