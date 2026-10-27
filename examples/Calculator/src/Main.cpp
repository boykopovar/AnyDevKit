#include "Display.hpp"
#include "Input.hpp"
#include "Interface.hpp"
#include "CalculatorError.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libkernel/include/Api.hpp"

#include <cstdint>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

int main() {
    try {
        constexpr int Width = ANYDEVKIT_CALCULATOR_WIDTH;
        constexpr int Height = 1080;
        Interface interface(Width, Height);
        Display display(Width, Height);
        Input input(Width, Height);
        for (;;) {
            interface.Tick();
            input.Poll(interface);
            if (interface.NeedsDraw()) {
                auto canvas = display.BackBuffer();
                interface.Draw(canvas);
                display.Present();
            } else display.Wait();
        }
    } catch (const CalculatorError& error) {
        char message[256];
        const auto length = snprintf_nid_postfix(message, sizeof(message), "Calculator: %s (0x%08x)\n", error.Message, static_cast<unsigned>(error.Code));
        if (length < 0 || static_cast<std::size_t>(length) >= sizeof(message)) throw CalculatorError{"Error message formatting failed"};
        check(sceKernelDebugOutText(0, message), "sceKernelDebugOutText");
        return error.Code;
    }
}
