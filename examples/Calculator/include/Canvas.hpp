#ifndef ANYDEVKIT_EXAMPLES_CALCULATOR_INCLUDE_CANVAS_HPP
#define ANYDEVKIT_EXAMPLES_CALCULATOR_INCLUDE_CANVAS_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

class Canvas final {
public:
    Canvas(std::span<std::uint32_t> pixels, int width, int height);
    static std::size_t StorageSize(int width, int height);
    void Rectangle(int x, int y, int width, int height, std::uint32_t color);
    void Text(int x, int y, std::string_view text, int scale, std::uint32_t color);
    void RoundedRectangle(int x, int y, int width, int height, int radius, std::uint32_t color);

private:
    std::span<std::uint32_t> pixels;
    int width;
    int height;
};

#endif
