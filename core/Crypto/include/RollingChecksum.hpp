#ifndef ANYDEVKIT_CRYPTO_ROLLINGCHECKSUM_HPP
#define ANYDEVKIT_CRYPTO_ROLLINGCHECKSUM_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyDevKit::Crypto {

class RollingChecksum final {
public:
    static std::uint64_t ByteSums(std::span<const std::byte> bytes);
    static std::uint64_t WindowHash(std::span<const std::byte> bytes);
};

}

#endif
