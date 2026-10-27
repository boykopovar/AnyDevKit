#ifndef ANYDEVKIT_CALCULATOR_INPUT_HPP
#define ANYDEVKIT_CALCULATOR_INPUT_HPP

#include "Interface.hpp"
#include <cstdint>

class Input final {
public:
    Input(int width, int height);
    void Poll(Interface& interface);

private:
    int pad;
    int mouse;
    int width;
    int height;
    int pointerX;
    int pointerY;
    int direction = 0;
    std::uint64_t nextRepeat = 0;
    std::uint32_t padButtons = 0;
    std::uint32_t mouseButtons = 0;
};

#endif
