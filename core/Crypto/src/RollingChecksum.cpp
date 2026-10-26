#include "RollingChecksum.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Crypto {

std::uint64_t RollingChecksum::ByteSums(std::span<const std::byte> bytes) {
    if (bytes.size() > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("Checksum input exceeds 32-bit length");
    std::uint32_t sum = 0;
    auto weighted = static_cast<std::uint32_t>(bytes.size());
    for (const auto byte : bytes) {
        sum += std::to_integer<std::uint32_t>(byte);
        weighted += sum;
    }
    return sum | (std::uint64_t{weighted} << 32);
}

std::uint64_t RollingChecksum::WindowHash(std::span<const std::byte> bytes) {
    std::uint64_t sum = 0, weighted = 0;
    const auto size = std::min<std::size_t>(0x10000, bytes.size());
    for (const auto byte : bytes.first(size)) {
        sum += std::to_integer<unsigned>(byte);
        weighted += sum;
    }
    weighted += (0x10000 - size) * sum;
    return sum ^ (weighted << 25);
}

}
