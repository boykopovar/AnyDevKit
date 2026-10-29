#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "Display.hpp"
#include "Graphics/Renderer.hpp"
#include "Input.hpp"
#include "Interface.hpp"
#include "CalculatorError.hpp"
#include <cstdio>

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
        Input input;
        Settings settings(input.UserId());
        Interface interface(Width, Height, settings);
        Display display(Width, Height);
        Renderer renderer(Width, Height);
        Canvas canvas(Width, Height);
        const auto start = sceKernelGetProcessTime();
        for (;;) {
            interface.Tick();
            input.Poll(interface);
            const auto seconds = static_cast<float>(sceKernelGetProcessTime() - start) / 1000000.0f;
            canvas.Clear();
            interface.Draw(canvas);
            renderer.Draw(display.Acquire(), seconds, canvas);
            display.Present();
        }
    } catch (const CalculatorError& error) {
        char message[256];
        const auto length = std::snprintf(message, sizeof(message), "Calculator: %s (0x%08x)\n", error.Message, static_cast<unsigned>(error.Code));
        if (length < 0 || static_cast<std::size_t>(length) >= sizeof(message)) throw CalculatorError{"Error message formatting failed"};
        check(sceKernelDebugOutText(0, message), "sceKernelDebugOutText");
        return error.Code;
    }
}
