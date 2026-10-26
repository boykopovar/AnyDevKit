#ifndef ANYDEVKIT_CRYPTO_MERSENNETWISTER_HPP
#define ANYDEVKIT_CRYPTO_MERSENNETWISTER_HPP

#include <array>
#include <cstdint>
#include <span>

namespace AnyDevKit::Crypto {

class MersenneTwister final {
public:
    explicit MersenneTwister(std::span<const std::uint32_t> seed);
    std::uint32_t Next();
private:
    std::array<std::uint32_t, 624> state{};
    std::size_t position = 624;
};

}

#endif
