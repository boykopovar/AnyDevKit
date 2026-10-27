#ifndef ANYDEVKIT_CALCULATOR_INPUT_HPP
#define ANYDEVKIT_CALCULATOR_INPUT_HPP

#include "Interface.hpp"
#include "SceTypes.hpp"
#include <array>
#include <atomic>
#include <cstdint>

class Input final {
public:
    Input();
    ~Input() noexcept(false);
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    void Poll(Interface& interface);
    int UserId() const;

private:
    struct Pulse { std::uint32_t Duration; std::uint8_t Low; std::uint8_t High; };
    static void* APS5_VABI vibrationEntry(void* argument);
    void runVibration();
    void vibrate(const Interface& interface);
    std::array<Pulse, 64> pulses{};
    std::atomic<std::size_t> pulseRead{0};
    std::atomic<std::size_t> pulseWrite{0};
    std::atomic<bool> stopping{false};
    std::atomic<int> vibrationError{0};
    Pthread vibrationThread{};
    int pad;
    int userId = 0;
    int direction = 0;
    std::uint64_t nextRepeat = 0;
    std::uint32_t padButtons = 0;
};

#endif
