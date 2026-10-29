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

Canvas::Canvas(int width, int height) : width(width), height(height) {
    StorageSize(width, height);
    commands = std::make_unique<DrawCommand[]>(Capacity);
}

std::size_t Canvas::StorageSize(int width, int height) {
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) throw CalculatorError{"Invalid canvas extent"};
    return static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
}

void Canvas::Clear() { count = 0; }
std::span<const DrawCommand> Canvas::Commands() const { return {commands.get(), count}; }

DrawCommand& Canvas::append(int x, int y, int rectangleWidth, int rectangleHeight, std::uint32_t color) {
    if (x < 0 || y < 0 || rectangleWidth < 0 || rectangleHeight < 0 || x > width || y > height || rectangleWidth > width - x || rectangleHeight > height - y) throw CalculatorError{"Rectangle outside canvas"};
    if (count == Capacity) throw CalculatorError{"Canvas draw command capacity exceeded"};
    auto& command = commands[count++];
    command = {};
    command.Rectangle = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(rectangleWidth), static_cast<float>(rectangleHeight)};
    command.Color = {static_cast<float>((color >> 16) & 255) / 255.0f, static_cast<float>((color >> 8) & 255) / 255.0f, static_cast<float>(color & 255) / 255.0f, static_cast<float>(color >> 24) / 255.0f};
    command.Parameters = {0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f};
    return command;
}

void Canvas::Rectangle(int x, int y, int rectangleWidth, int rectangleHeight, std::uint32_t color) {
    append(x, y, rectangleWidth, rectangleHeight, color);
}

void Canvas::Text(int x, int y, std::string_view text, int scale, std::uint32_t color) {
    if (scale <= 0 || scale > 16) throw CalculatorError{"Invalid text scale"};
    for (char character : text) {
        const auto rows = glyph(character);
        if (character != ' ') {
            auto& command = append(x, y, 5 * scale, 7 * scale, color);
            command.Parameters[3] = static_cast<float>(scale);
            for (std::size_t row = 0; row < rows.size(); ++row) command.Glyph[row / 4] |= rows[row] << ((row % 4) * 5);
        }
        x += 6 * scale;
    }
}

void Canvas::RoundedRectangle(int x, int y, int rectangleWidth, int rectangleHeight, int radius, std::uint32_t color) {
    if (radius < 0 || radius > rectangleWidth / 2 || radius > rectangleHeight / 2) throw CalculatorError{"Invalid corner radius"};
    append(x, y, rectangleWidth, rectangleHeight, color).Parameters[0] = static_cast<float>(radius);
}
