#ifndef ANYDEVKIT_CALCULATOR_DISPLAY_HPP
#define ANYDEVKIT_CALCULATOR_DISPLAY_HPP

#include "Canvas.hpp"
#include <array>

class Display final {
public:
    Display(int width, int height);
    ~Display() noexcept(false);
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    Canvas BackBuffer();
    void SetBackground(std::span<const std::uint32_t> pixels);
    void Present();
    void Wait();

private:
    int width;
    int height;
    int handle;
    int back = 0;
    std::int64_t sequence = 0;
    std::uint64_t* labels = nullptr;
    std::span<std::uint32_t> frame;
    std::array<std::span<std::uint32_t>, 2> buffers;
};

#endif
