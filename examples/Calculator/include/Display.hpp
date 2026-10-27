#ifndef ANYDEVKIT_CALCULATOR_DISPLAY_HPP
#define ANYDEVKIT_CALCULATOR_DISPLAY_HPP

#include "Canvas.hpp"
#include <array>

class Display final {
public:
    Display(int width, int height);
    Canvas BackBuffer();
    void Present();
    void Wait();

private:
    int width;
    int height;
    int handle;
    int back = 0;
    std::int64_t sequence = 0;
    std::array<std::span<std::uint32_t>, 2> buffers;
};

#endif
