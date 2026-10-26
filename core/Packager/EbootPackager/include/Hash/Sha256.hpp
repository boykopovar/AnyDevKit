#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace AnyDevKit::Self {

class Sha256 final {
public:
    static std::array<std::byte, 32> Compute(std::span<const std::byte> bytes);
};

}
