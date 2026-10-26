#ifndef ANYDEVKIT_CORE_CRYPTO_CRC32_HPP
#define ANYDEVKIT_CORE_CRYPTO_CRC32_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyDevKit::Crypto {

class Crc32 final {
public:
    static std::uint32_t Zip(std::span<const std::byte> bytes);
    static std::uint32_t Castagnoli(std::span<const std::byte> bytes);
private:
    static std::uint32_t compute(std::span<const std::byte> bytes, std::uint32_t polynomial);
};

}

#endif
