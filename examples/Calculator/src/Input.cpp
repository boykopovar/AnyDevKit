#include "Input.hpp"
#include "CalculatorError.hpp"
#include "prx/libScePad/libScePad.hpp"
#include "prx/libSceMouse/libSceMouse.hpp"
#include "prx/libSceUserService/libSceUserService.hpp"
#include "prx/libkernel/Time/include/Time.hpp"

#include <algorithm>
#include <array>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

Input::Input(int width, int height) : width(width), height(height), pointerX(width / 2), pointerY(height / 2) {
    check(sceUserServiceInitialize(nullptr), "sceUserServiceInitialize");
    int user = 0;
    check(sceUserServiceGetInitialUser(&user), "sceUserServiceGetInitialUser");
    check(scePadInit_nid_postfix(), "scePadInit");
    pad = check(scePadOpen_nid_postfix(user, 0, 0, nullptr), "scePadOpen");
    check(sceMouseInit(), "sceMouseInit");
    mouse = check(sceMouseOpen(user, 0, 0, nullptr), "sceMouseOpen");
}

void Input::Poll(Interface& interface) {
    std::array<MouseData, 64> events{};
    const int count = check(sceMouseRead(mouse, events.data(), static_cast<int>(events.size())), "sceMouseRead");
    if (count > static_cast<int>(events.size())) throw CalculatorError{"Invalid mouse event count"};
    for (int index = 0; index < count; ++index) {
        const auto& event = events[index];
        if (!event.connected) { mouseButtons = 0; continue; }
        if (event.x_axis != 0 || event.y_axis != 0 || event.buttons != mouseButtons) {
            pointerX = static_cast<int>(std::clamp<std::int64_t>(static_cast<std::int64_t>(pointerX) + event.x_axis, 0, width - 20));
            pointerY = static_cast<int>(std::clamp<std::int64_t>(static_cast<std::int64_t>(pointerY) + event.y_axis, 0, height - 24));
            interface.Point(pointerX, pointerY);
        }
        if ((event.buttons & ~mouseButtons & 1u) != 0) interface.Click();
        mouseButtons = event.buttons;
    }
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
    if (data.touch_data_touch_num != 0) {
        PadControllerInformation information{};
        check(scePadGetControllerInformation(pad, &information), "scePadGetControllerInformation");
        const auto resolution = information.touchPadInfo.resolution;
        if (resolution.x == 0 || resolution.y == 0) throw CalculatorError{"Invalid touchpad resolution"};
        pointerX = std::clamp(static_cast<int>(data.touch_data_touch0_x) * width / resolution.x, 0, width - 20);
        pointerY = std::clamp(static_cast<int>(data.touch_data_touch0_y) * height / resolution.y, 0, height - 24);
        interface.Point(pointerX, pointerY);
        if ((pressed & 0x100000u) != 0) interface.Click();
    }
    if ((pressed & 0x4000u) != 0) interface.Activate();
    if ((pressed & 0x2000u) != 0) interface.Press(CalculatorKey::Backspace);
    if ((pressed & 0x1000u) != 0) interface.Press(CalculatorKey::Clear);
    if ((pressed & 0x8000u) != 0) interface.Press(CalculatorKey::Equals);
    padButtons = data.buttons;
}
