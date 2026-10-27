#include "Input.hpp"
#include "CalculatorError.hpp"
#include "prx/libScePad/libScePad.hpp"
#include "prx/libSceSysmodule/libSceSysmodule.hpp"
#include "prx/libSceUserService/libSceUserService.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

Input::Input() {
    check(sceSysmoduleLoadModuleInternal(0x80000011), "Load UserService");
    check(sceSysmoduleLoadModuleInternal(0x80000024), "Load Pad");
    check(sceUserServiceInitialize(nullptr), "sceUserServiceInitialize");
    check(sceUserServiceGetInitialUser(&userId), "sceUserServiceGetInitialUser");
    check(scePadInit_nid_postfix(), "scePadInit");
    pad = check(scePadOpen_nid_postfix(userId, 0, 0, nullptr), "scePadOpen");
    check(scePadSetVibrationMode(pad, 2), "scePadSetVibrationMode");
    check(scePthreadCreate(&vibrationThread, nullptr, vibrationEntry, this, "CalculatorVibration"), "scePthreadCreate");
}

int Input::UserId() const { return userId; }

Input::~Input() noexcept(false) {
    stopping.store(true, std::memory_order_release);
    check(scePthreadJoin(vibrationThread, nullptr), "scePthreadJoin");
}

void* APS5_VABI Input::vibrationEntry(void* argument) {
    auto& input = *static_cast<Input*>(argument);
    try {
        input.runVibration();
    } catch (const CalculatorError& error) {
        input.vibrationError.store(error.Code, std::memory_order_release);
    }
    const PadVibrationParam stopped{};
    const int result = scePadSetVibration(input.pad, &stopped);
    if (result < 0) input.vibrationError.store(result, std::memory_order_release);
    return nullptr;
}

void Input::runVibration() {
    while (!stopping.load(std::memory_order_acquire)) {
        const auto read = pulseRead.load(std::memory_order_relaxed);
        if (read == pulseWrite.load(std::memory_order_acquire)) {
            check(sceKernelUsleep_nid_postfix(1000), "sceKernelUsleep");
            continue;
        }
        const auto pulse = pulses[read % pulses.size()];
        pulseRead.store(read + 1, std::memory_order_release);
        if (pulse.Duration == 0 || (pulse.Low == 0 && pulse.High == 0)) continue;
        const PadVibrationParam vibration{pulse.Low, pulse.High};
        check(scePadSetVibration(pad, &vibration), "scePadSetVibration");
        check(sceKernelUsleep_nid_postfix(pulse.Duration), "sceKernelUsleep");
        const PadVibrationParam stopped{};
        check(scePadSetVibration(pad, &stopped), "scePadSetVibration");
        check(sceKernelUsleep_nid_postfix(5000), "sceKernelUsleep");
    }
}

void Input::vibrate(const Interface& interface) {
    const auto write = pulseWrite.load(std::memory_order_relaxed);
    if (write - pulseRead.load(std::memory_order_acquire) == pulses.size()) throw CalculatorError{"Vibration queue is full"};
    pulses[write % pulses.size()] = {static_cast<std::uint32_t>(interface.VibrationDuration()) * 1000, static_cast<std::uint8_t>(interface.LowFrequencyStrength()), static_cast<std::uint8_t>(interface.HighFrequencyStrength())};
    pulseWrite.store(write + 1, std::memory_order_release);
}

void Input::Poll(Interface& interface) {
    const int error = vibrationError.load(std::memory_order_acquire);
    if (error != 0) throw CalculatorError{"Vibration worker failed", error};
    PadData data{};
    check(scePadReadState(pad, &data), "scePadReadState");
    if (!data.connected) { padButtons = 0; direction = 0; return; }
    const auto pressed = data.buttons & ~padButtons;
    const int horizontal = ((data.buttons & 0x20u) != 0 || data.left_stick_x > 192) - ((data.buttons & 0x80u) != 0 || data.left_stick_x < 64);
    const int vertical = ((data.buttons & 0x40u) != 0 || data.left_stick_y > 192) - ((data.buttons & 0x10u) != 0 || data.left_stick_y < 64);
    const int nextDirection = horizontal != 0 ? horizontal : vertical * 2;
    const auto now = sceKernelGetProcessTime();
    if (nextDirection != 0 && (nextDirection != direction || now >= nextRepeat)) {
        interface.Navigate(horizontal, horizontal == 0 ? vertical : 0);
        vibrate(interface);
        nextRepeat = now + (nextDirection != direction ? 350000 : 110000);
    }
    direction = nextDirection;
    if ((pressed & 0x4000u) != 0) { interface.Activate(); vibrate(interface); }
    if ((pressed & 0x2000u) != 0) { interface.Press(CalculatorKey::Backspace); vibrate(interface); }
    if ((pressed & 0x1000u) != 0) { interface.Press(CalculatorKey::Clear); vibrate(interface); }
    if ((pressed & 0x8000u) != 0) { interface.Press(CalculatorKey::Equals); vibrate(interface); }
    padButtons = data.buttons;
}
