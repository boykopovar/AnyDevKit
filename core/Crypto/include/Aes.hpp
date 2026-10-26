#ifndef ANYDEVKIT_CORE_CRYPTO_AES_HPP
#define ANYDEVKIT_CORE_CRYPTO_AES_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyDevKit::Crypto {

class Aes final {
public:
    explicit Aes(std::span<const std::byte> key);
    std::array<std::byte, 16> EncryptBlock(std::span<const std::byte, 16> block) const;
    std::array<std::byte, 16> DecryptBlock(std::span<const std::byte, 16> block) const;
    void EncryptCbc(std::span<std::byte> bytes, std::span<const std::byte, 16> iv) const;
    void DecryptCbc(std::span<std::byte> bytes, std::span<const std::byte, 16> iv) const;
    static void EncryptXts(std::span<std::byte> bytes, std::span<const std::byte> key, std::span<const std::byte, 16> tweak);
    static void DecryptXts(std::span<std::byte> bytes, std::span<const std::byte> key, std::span<const std::byte, 16> tweak);
private:
    std::array<std::byte, 16> transform(std::span<const std::byte, 16> block, bool decrypt) const;
    static void transformXts(std::span<std::byte> bytes, std::span<const std::byte> key, std::span<const std::byte, 16> tweak, bool decrypt);
    std::array<std::uint8_t, 240> roundKeys{};
    std::size_t rounds = 0;
};

}

#endif
