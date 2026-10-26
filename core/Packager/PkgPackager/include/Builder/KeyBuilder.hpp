#ifndef ANYDEVKIT_PKG_KEYBUILDER_HPP
#define ANYDEVKIT_PKG_KEYBUILDER_HPP

#include "Domain/PackageKeys.hpp"

#include <string_view>
#include <vector>

namespace AnyDevKit::Pkg {

struct PackageKeyData {
    std::array<std::byte, 32> ImageSecret{};
    std::vector<std::byte> EntryKeys;
    std::vector<std::byte> ImageKey;
};

class KeyBuilder final {
public:
    static PackageKeyData Build(std::string_view contentId, const PackageKeys& keys);
    static std::array<std::byte, 384> Wrap(std::span<const std::byte> message, std::span<const std::byte, 384> modulus);
};

}

#endif
