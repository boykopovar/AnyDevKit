#ifndef ANYDEVKIT_PACKAGER_CRYPTO_RSA3072_HPP
#define ANYDEVKIT_PACKAGER_CRYPTO_RSA3072_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyDevKit::Crypto {

class Rsa3072 final {
public:
    static std::array<std::byte, 384> Encrypt(std::span<const std::byte> message, std::span<const std::byte, 384> modulus, std::uint32_t exponent, std::span<const std::byte> padding);
    static std::array<std::byte, 384> ApplyPublic(std::span<const std::byte, 384> message, std::span<const std::byte, 384> modulus, std::uint32_t exponent);
};

}

#endif
