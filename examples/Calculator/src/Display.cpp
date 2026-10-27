#include "Display.hpp"
#include "CalculatorError.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
#include "prx/libkernel/libkernel.hpp"

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

Display::Display(int width, int height) : width(width), height(height) {
    constexpr std::size_t Alignment = 65536;
    const auto capacity = Canvas::StorageSize(width, height);
    std::int64_t physicalAddress = 0;
    check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), capacity * 2, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
    void* address = nullptr;
    const auto result = sceKernelMapDirectMemory(&address, capacity * 2, 0x33, 0, physicalAddress, Alignment);
    if (result < 0) {
        check(sceKernelReleaseDirectMemory(physicalAddress, capacity * 2), "sceKernelReleaseDirectMemory");
        check(result, "sceKernelMapDirectMemory");
    }
    VideoOutBuffers registered[2]{};
    for (std::size_t index = 0; index < buffers.size(); ++index) {
        auto* pixels = reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(address) + index * capacity);
        buffers[index] = std::span(pixels, capacity / sizeof(std::uint32_t));
        registered[index].data = pixels;
    }
    handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
    VideoOutBufferAttribute2 attribute{};
    sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 0, width, height, 0, 0, 0);
    check(sceVideoOutRegisterBuffers2(handle, 0, 0, registered, 2, &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
}

Canvas Display::BackBuffer() { return Canvas(buffers[back], width, height); }
void Display::Wait() { check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank"); }

void Display::Present() {
    check(sceVideoOutSubmitFlip(handle, back, 1, ++sequence), "sceVideoOutSubmitFlip");
    const auto start = sceKernelGetProcessTime();
    for (;;) {
        Wait();
        VideoOutFlipStatus status{};
        check(sceVideoOutGetFlipStatus(handle, &status), "sceVideoOutGetFlipStatus");
        if (status.flipArg == sequence && status.currentBuffer == back) break;
        if (sceKernelGetProcessTime() - start > 5000000) throw CalculatorError{"Display flip timed out"};
    }
    back = 1 - back;
}

