#ifndef ANYDEVKIT_CORE_CRYPTO_SHA3_HPP
#define ANYDEVKIT_CORE_CRYPTO_SHA3_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyDevKit::Crypto {

class Sha3 final {
public:
    void Update(std::span<const std::byte> bytes);
    std::array<std::byte, 32> Finish();
    static std::array<std::byte, 32> Compute(std::span<const std::byte> bytes);
    static std::array<std::byte, 32> Hmac(std::span<const std::byte> key, std::span<const std::byte> bytes);
private:
    void permute();
    std::array<std::uint64_t, 25> state{};
    std::size_t position = 0;
    bool finished = false;
};

}

#endif
