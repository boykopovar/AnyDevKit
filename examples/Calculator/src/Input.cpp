#include "Input.hpp"
#include "CalculatorError.hpp"
#include "prx/libScePad/libScePad.hpp"
#include "prx/libSceSysmodule/libSceSysmodule.hpp"
#include "prx/libSceUserService/libSceUserService.hpp"
#include "prx/libkernel/Time/include/Time.hpp"

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
    int user = 0;
    check(sceUserServiceGetInitialUser(&user), "sceUserServiceGetInitialUser");
    check(scePadInit_nid_postfix(), "scePadInit");
    pad = check(scePadOpen_nid_postfix(user, 0, 0, nullptr), "scePadOpen");
}

void Input::Poll(Interface& interface) {
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
        nextRepeat = now + (nextDirection != direction ? 350000 : 110000);
    }
    direction = nextDirection;
    if ((pressed & 0x4000u) != 0) interface.Activate();
    if ((pressed & 0x2000u) != 0) interface.Press(CalculatorKey::Backspace);
    if ((pressed & 0x1000u) != 0) interface.Press(CalculatorKey::Clear);
    if ((pressed & 0x8000u) != 0) interface.Press(CalculatorKey::Equals);
    padButtons = data.buttons;
}
