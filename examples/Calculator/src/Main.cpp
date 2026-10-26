#include "Canvas.hpp"
#include "CalculatorError.hpp"
#include "Runtime.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
#include "prx/libc/include/Api.hpp"

#include <cstdint>
#include <memory>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

int RunCalculator() {
    try {
        constexpr int width = ANYDEVKIT_CALCULATOR_WIDTH;
        if (width < 480 || width > 16384) throw CalculatorError{"Calculator width must be between 480 and 16384"};
        constexpr int Height = 704;
        constexpr std::size_t Alignment = 65536;
        const auto capacity = width * Height + Alignment / sizeof(std::uint32_t);
        std::unique_ptr<std::uint32_t, decltype(&free_nid_postfix)> storage(static_cast<std::uint32_t*>(malloc_nid_postfix(capacity * sizeof(std::uint32_t))), free_nid_postfix);
        if (storage == nullptr) throw CalculatorError{"Framebuffer allocation failed"};
        const auto address = reinterpret_cast<std::uintptr_t>(storage.get());
        const auto offset = ((Alignment - address % Alignment) % Alignment) / sizeof(std::uint32_t);
        auto pixels = std::span(storage.get() + offset, width * Height);
        Canvas(pixels, width, Height).DrawCalculator();
        const int handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
        VideoOutBufferAttribute2 attribute{};
        sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 1, width, Height, 0, 0, 0);
        VideoOutBuffers buffer{pixels.data(), nullptr, {nullptr, nullptr}};
        check(sceVideoOutRegisterBuffers2(handle, 0, 0, &buffer, 1, &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
        check(sceVideoOutSubmitFlip(handle, 0, 1, 1), "sceVideoOutSubmitFlip");
        for (;;) check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank");
    } catch (const CalculatorError& error) {
        printf_nid_postfix("Calculator: %s (%d)\n", error.Message, error.Code);
        return 1;
    }
}

extern "C" void CalculatorEntry() {
    InitializeRuntime();
    exit_nid_postfix(RunCalculator());
}
