#include "Hash/Crc32.hpp"

namespace AnyDevKit::Pkg {

std::uint32_t Crc32::compute(std::span<const std::byte> bytes, std::uint32_t polynomial) {
    std::uint32_t result = 0xffffffff;
    for (const auto byte : bytes) {
        result ^= std::to_integer<std::uint32_t>(byte);
        for (int bit = 0; bit < 8; ++bit) result = (result >> 1) ^ ((0u - (result & 1u)) & polynomial);
    }
    return result ^ 0xffffffff;
}

std::uint32_t Crc32::Zip(std::span<const std::byte> bytes) {
    return compute(bytes, 0xedb88320);
}

std::uint32_t Crc32::Castagnoli(std::span<const std::byte> bytes) {
    return compute(bytes, 0x82f63b78);
}

}
