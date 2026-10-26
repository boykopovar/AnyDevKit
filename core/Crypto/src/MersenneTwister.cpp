#include "MersenneTwister.hpp"

#include <algorithm>
#include <stdexcept>

namespace AnyDevKit::Crypto {

MersenneTwister::MersenneTwister(std::span<const std::uint32_t> seed) {
    if (seed.empty()) throw std::invalid_argument("Mersenne Twister seed is empty");
    state[0] = 19650218;
    for (std::size_t i = 1; i < state.size(); ++i) state[i] = 1812433253u * (state[i - 1] ^ (state[i - 1] >> 30)) + static_cast<std::uint32_t>(i);
    std::size_t i = 1, j = 0;
    for (std::size_t count = std::max(state.size(), seed.size()); count; --count) {
        state[i] = (state[i] ^ ((state[i - 1] ^ (state[i - 1] >> 30)) * 1664525u)) + seed[j] + static_cast<std::uint32_t>(j);
        if (++i == state.size()) {
            state[0] = state.back();
            i = 1;
        }
        if (++j == seed.size()) j = 0;
    }
    for (std::size_t count = state.size() - 1; count; --count) {
        state[i] = (state[i] ^ ((state[i - 1] ^ (state[i - 1] >> 30)) * 1566083941u)) - static_cast<std::uint32_t>(i);
        if (++i == state.size()) {
            state[0] = state.back();
            i = 1;
        }
    }
    state[0] = 0x80000000;
}

std::uint32_t MersenneTwister::Next() {
    if (position == state.size()) {
        for (std::size_t i = 0; i < state.size(); ++i) {
            const auto combined = (state[i] & 0x80000000u) | (state[(i + 1) % state.size()] & 0x7fffffffu);
            state[i] = state[(i + 397) % state.size()] ^ (combined >> 1) ^ ((combined & 1) ? 0x9908b0dfu : 0);
        }
        position = 0;
    }
    auto value = state[position++];
    value ^= value >> 11;
    value ^= (value << 7) & 0x9d2c5680;
    value ^= (value << 15) & 0xefc60000;
    return value ^ (value >> 18);
}

}
