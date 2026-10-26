#ifndef ANYDEVKIT_CORE_PACKAGER_EBOOTPACKAGER_INCLUDE_HASH_SHA256_HPP
#define ANYDEVKIT_CORE_PACKAGER_EBOOTPACKAGER_INCLUDE_HASH_SHA256_HPP

#include <array>
#include <cstddef>
#include <span>

namespace AnyDevKit::Self {

class Sha256 final {
public:
    static std::array<std::byte, 32> Compute(std::span<const std::byte> bytes);
};

}

#endif
