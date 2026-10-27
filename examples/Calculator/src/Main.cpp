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
        constexpr int Height = 1080;
        constexpr std::size_t Alignment = 65536;
        const auto capacity = Canvas::StorageSize(width, Height);
        std::int64_t physicalAddress = 0;
        check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), capacity * 2, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
        void* address = nullptr;
        const auto mapResult = sceKernelMapDirectMemory(&address, capacity * 2, 0x33, 0, physicalAddress, Alignment);
        if (mapResult < 0) {
            check(sceKernelReleaseDirectMemory(physicalAddress, capacity * 2), "sceKernelReleaseDirectMemory");
            check(mapResult, "sceKernelMapDirectMemory");
        }
        VideoOutBuffers buffers[2]{};
        for (std::size_t index = 0; index < 2; ++index) {
            auto* pixels = reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(address) + index * capacity);
            Canvas(std::span(pixels, capacity / sizeof(std::uint32_t)), width, Height).DrawCalculator();
            buffers[index].data = pixels;
        }
        const int handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
        VideoOutBufferAttribute2 attribute{};
        sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 0, width, Height, 0, 0, 0);
        check(sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 2, &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
        check(sceVideoOutSubmitFlip(handle, 0, 1, 1), "sceVideoOutSubmitFlip");
        for (;;) check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank");
    } catch (const CalculatorError& error) {
        char message[256];
        const auto length = snprintf_nid_postfix(message, sizeof(message), "Calculator: %s (0x%08x)\n", error.Message, static_cast<unsigned>(error.Code));
        if (length < 0 || static_cast<std::size_t>(length) >= sizeof(message)) throw CalculatorError{"Error message formatting failed"};
        check(sceKernelDebugOutText(0, message), "sceKernelDebugOutText");
        return error.Code;
    }
}
