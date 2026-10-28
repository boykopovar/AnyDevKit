#ifndef ANYDEVKIT_CALCULATOR_DISPLAY_HPP
#define ANYDEVKIT_CALCULATOR_DISPLAY_HPP

#include "Canvas.hpp"
#include <array>

class Display final {
public:
    Display(int width, int height);
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    std::span<std::uint32_t> Acquire();
    void Present();
    void Wait();

private:
    int handle;
    int back = 0;
    std::int64_t sequence = 0;
    std::uint64_t* labels = nullptr;
    bool acquired = false;
    std::array<std::span<std::uint32_t>, 2> buffers;
};

#endif
