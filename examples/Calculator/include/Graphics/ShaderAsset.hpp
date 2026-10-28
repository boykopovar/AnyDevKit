#ifndef ANYDEVKIT_CALCULATOR_GRAPHICS_SHADERASSET_HPP
#define ANYDEVKIT_CALCULATOR_GRAPHICS_SHADERASSET_HPP

#include <array>
#include <cstdint>
#include <span>

struct ShaderAsset {
    std::span<const std::uint8_t> Header;
    std::span<const std::uint8_t> Code;
    std::array<std::uint32_t, 16> UserData;
    std::uint32_t UserDataCount;
    int PushConstantSlot;
};

#endif
