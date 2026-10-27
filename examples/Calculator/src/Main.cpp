#include "Canvas.hpp"
#include "CalculatorError.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
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
        constexpr int width = ANYDEVKIT_CALCULATOR_WIDTH;
        if (width < 480 || width > 16384) throw CalculatorError{"Calculator width must be between 480 and 16384"};
        constexpr int Height = 704;
        constexpr std::size_t Alignment = 65536;
        constexpr auto capacity = (static_cast<std::size_t>(width) * Height * sizeof(std::uint32_t) + Alignment - 1) & ~(Alignment - 1);
        std::int64_t physicalAddress = 0;
        check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), capacity, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
        void* address = nullptr;
        const auto mapResult = sceKernelMapDirectMemory(&address, capacity, 0x33, 0, physicalAddress, Alignment);
        if (mapResult < 0) {
            check(sceKernelReleaseDirectMemory(physicalAddress, capacity), "sceKernelReleaseDirectMemory");
            check(mapResult, "sceKernelMapDirectMemory");
        }
        auto pixels = std::span(static_cast<std::uint32_t*>(address), width * Height);
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
