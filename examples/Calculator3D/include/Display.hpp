#ifndef ANYDEVKIT_CALCULATOR3D_DISPLAY_HPP
#define ANYDEVKIT_CALCULATOR3D_DISPLAY_HPP

#include "Canvas.hpp"
#include <array>

class Display final {
public:
    struct Flip {
        int Handle;
        int BufferIndex;
        std::int64_t Argument;
    };

    Display(int width, int height);
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    std::span<std::uint32_t> Acquire();
    Flip PrepareFlip();
    void Present();
    void Wait();

private:
    static constexpr std::size_t bufferCount = 4;
    int handle;
    int back = 0;
    std::int64_t sequence = 0;
    std::uint64_t* labels = nullptr;
    bool acquired = false;
    std::array<std::span<std::uint32_t>, bufferCount> buffers;
};

#endif
