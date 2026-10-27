#ifndef ANYDEVKIT_CALCULATOR_INPUT_HPP
#define ANYDEVKIT_CALCULATOR_INPUT_HPP

#include "Interface.hpp"
#include <cstdint>

class Input final {
public:
    Input();
    void Poll(Interface& interface);

private:
    void vibrate(std::uint64_t now, const Interface& interface);
    int pad;
    int direction = 0;
    std::uint64_t nextRepeat = 0;
    std::uint64_t vibrationEnd = 0;
    std::uint32_t padButtons = 0;
};

#endif
