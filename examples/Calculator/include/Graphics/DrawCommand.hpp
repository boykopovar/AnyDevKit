#ifndef ANYDEVKIT_CALCULATOR_GRAPHICS_DRAWCOMMAND_HPP
#define ANYDEVKIT_CALCULATOR_GRAPHICS_DRAWCOMMAND_HPP

#include <array>
#include <cstdint>

struct alignas(16) DrawCommand {
    std::array<float, 4> Rectangle;
    std::array<float, 4> Color;
    std::array<float, 4> Parameters;
    std::array<std::uint32_t, 4> Glyph;
};

static_assert(sizeof(DrawCommand) == 64);

#endif
