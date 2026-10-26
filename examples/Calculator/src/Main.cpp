#include "Canvas.hpp"
#include "CalculatorError.hpp"
#include "PeRelocations.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
#include "prx/libc/include/Api.hpp"

#include <cstdint>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

int RunCalculator() {
    try {
        constexpr int Width = 480;
        constexpr int Height = 704;
        constexpr std::size_t Alignment = 65536;
        const auto capacity = Width * Height + Alignment / sizeof(std::uint32_t);
        auto* storage = static_cast<std::uint32_t*>(malloc_nid_postfix(capacity * sizeof(std::uint32_t)));
        if (storage == nullptr) throw CalculatorError{"Framebuffer allocation failed"};
        const auto address = reinterpret_cast<std::uintptr_t>(storage);
        const auto offset = ((Alignment - address % Alignment) % Alignment) / sizeof(std::uint32_t);
        auto pixels = std::span(storage + offset, Width * Height);
        Canvas(pixels, Width, Height).DrawCalculator();
        const int handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
        VideoOutBufferAttribute2 attribute{};
        sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 1, Width, Height, 0, 0, 0);
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
    _pei386_runtime_relocator();
    exit_nid_postfix(RunCalculator());
}
