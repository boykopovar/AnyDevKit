#ifndef ANYDEVKIT_EXAMPLES_CALCULATOR_INCLUDE_CANVAS_HPP
#define ANYDEVKIT_EXAMPLES_CALCULATOR_INCLUDE_CANVAS_HPP

#include "Graphics/DrawCommand.hpp"
#include <cstddef>
#include <span>
#include <memory>
#include <string_view>

class Canvas final {
public:
    static constexpr std::size_t Capacity = 1024;
    Canvas(int width, int height);
    static std::size_t StorageSize(int width, int height);
    void Clear();
    std::span<const DrawCommand> Commands() const;
    void Rectangle(int x, int y, int width, int height, std::uint32_t color);
    void Text(int x, int y, std::string_view text, int scale, std::uint32_t color);
    void RoundedRectangle(int x, int y, int width, int height, int radius, std::uint32_t color);

private:
    DrawCommand& append(int x, int y, int width, int height, std::uint32_t color);
    std::unique_ptr<DrawCommand[]> commands;
    std::size_t count = 0;
    int width;
    int height;
};

#endif
