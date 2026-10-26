#ifndef ANYDEVKIT_CORE_CRYPTO_SHA256_HPP
#define ANYDEVKIT_CORE_CRYPTO_SHA256_HPP

#include <array>
#include <cstddef>
#include <span>

namespace AnyDevKit::Crypto {

class Sha256 final {
public:
    static std::array<std::byte, 32> Compute(std::span<const std::byte> bytes);
    static std::array<std::byte, 32> Hmac(std::span<const std::byte> key, std::span<const std::byte> bytes);
};

}

#endif
