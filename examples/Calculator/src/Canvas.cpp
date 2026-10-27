#include "Canvas.hpp"
#include "CalculatorError.hpp"

#include <array>

namespace {

std::array<unsigned, 7> glyph(char character) {
    switch (character) {
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'W': return {17, 17, 17, 21, 21, 27, 17};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '%': return {25, 25, 2, 4, 8, 19, 19};
    case ':': return {0, 6, 6, 0, 6, 6, 0};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'I': return {14, 4, 4, 4, 4, 4, 14};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'N': return {17, 25, 25, 21, 19, 19, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '*': return {0, 17, 10, 4, 10, 17, 0};
    case '/': return {1, 2, 2, 4, 8, 8, 16};
    case '=': return {0, 0, 31, 0, 31, 0, 0};
    case '.': return {0, 0, 0, 0, 0, 6, 6};
    case ' ': return {};
    default: throw CalculatorError{"Unsupported glyph"};
    }
}

}

Canvas::Canvas(std::span<std::uint32_t> pixels, int width, int height) : pixels(pixels), width(width), height(height) {
    if (pixels.size_bytes() != StorageSize(width, height)) throw CalculatorError{"Invalid canvas extent"};
}

std::size_t Canvas::StorageSize(int width, int height) {
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) throw CalculatorError{"Invalid canvas extent"};
    return static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
}

void Canvas::Rectangle(int x, int y, int rectangleWidth, int rectangleHeight, std::uint32_t color) {
    if (x < 0 || y < 0 || rectangleWidth < 0 || rectangleHeight < 0 || x > width || y > height || rectangleWidth > width - x || rectangleHeight > height - y) throw CalculatorError{"Rectangle outside canvas"};
    for (int row = y; row < y + rectangleHeight; ++row) {
        for (int column = x; column < x + rectangleWidth; ++column) {
            const auto tile = static_cast<std::size_t>(row / 128) * ((width + 127) / 128) + column / 128;
            const auto tileX = static_cast<std::uint32_t>(column);
            const auto tileY = static_cast<std::uint32_t>(row);
            const auto offset = ((tileY << 4) & 0x0070u) ^ ((tileY << 5) & 0x0f00u) ^ ((tileY << 9) & 0x1000u) ^ ((tileY << 8) & 0x4000u) ^ ((tileX << 2) & 0x000cu) ^ ((tileX << 5) & 0x0380u) ^ ((tileX << 4) & 0x0400u) ^ ((tileX << 6) & 0x0800u) ^ ((tileX << 9) & 0xa000u);
            pixels[tile * 16384 + offset / sizeof(std::uint32_t)] = color;
        }
    }
}

void Canvas::Text(int x, int y, std::string_view text, int scale, std::uint32_t color) {
    if (scale <= 0 || scale > 16) throw CalculatorError{"Invalid text scale"};
    for (char character : text) {
        const auto rows = glyph(character);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[row] & (1u << (4 - column))) != 0) Rectangle(x + column * scale, y + row * scale, scale, scale, color);
            }
        }
        x += 6 * scale;
    }
}

void Canvas::RoundedRectangle(int x, int y, int rectangleWidth, int rectangleHeight, int radius, std::uint32_t color) {
    if (radius < 0 || radius * 2 > rectangleWidth || radius * 2 > rectangleHeight) throw CalculatorError{"Invalid corner radius"};
    Rectangle(x, y + radius, rectangleWidth, rectangleHeight - radius * 2, color);
    for (int row = 0; row < radius; ++row) {
        const int dy = radius - row - 1;
        int inset = 0;
        while ((radius - inset - 1) * (radius - inset - 1) + dy * dy > radius * radius) ++inset;
        Rectangle(x + inset, y + row, rectangleWidth - inset * 2, 1, color);
        Rectangle(x + inset, y + rectangleHeight - row - 1, rectangleWidth - inset * 2, 1, color);
    }
}
