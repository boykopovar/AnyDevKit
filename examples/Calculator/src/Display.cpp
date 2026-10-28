#include "Display.hpp"
#include "CalculatorError.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
#include "prx/libkernel/libkernel.hpp"
#include "prx/libc/include/Api.hpp"

#include <atomic>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

}

Display::Display(int width, int height) {
    constexpr std::size_t Alignment = 131072;
    const auto capacity = Canvas::StorageSize(width, height);
    const auto stride = (capacity + Alignment - 1) & ~(Alignment - 1);
    std::int64_t physicalAddress = 0;
    check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), stride * 2, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
    void* address = nullptr;
    const auto result = sceKernelMapDirectMemory(&address, stride * 2, 0x33, 0, physicalAddress, Alignment);
    if (result < 0) {
        check(sceKernelReleaseDirectMemory(physicalAddress, stride * 2), "sceKernelReleaseDirectMemory");
        check(result, "sceKernelMapDirectMemory");
    }
    VideoOutBuffers registered[2]{};
    for (std::size_t index = 0; index < buffers.size(); ++index) {
        auto* pixels = reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(address) + index * stride);
        buffers[index] = std::span(pixels, capacity / sizeof(std::uint32_t));
        registered[index].data = pixels;
    }
    handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
    VideoOutBufferAttribute2 attribute{};
    sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 0, width, height, 0, 0, 0);
    check(sceVideoOutRegisterBuffers2(handle, 0, 0, registered, 2, &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
    std::uintptr_t labelAddress = 0;
    const auto labelCount = check(sceVideoOutGetBufferLabelAddress(handle, &labelAddress), "sceVideoOutGetBufferLabelAddress");
    if (labelCount < static_cast<int>(buffers.size()) || labelAddress == 0 || labelAddress % std::atomic_ref<std::uint64_t>::required_alignment != 0) throw CalculatorError{"Invalid VideoOut buffer labels"};
    labels = reinterpret_cast<std::uint64_t*>(labelAddress);
}

void Display::Wait() { check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank"); }

std::span<std::uint32_t> Display::Acquire() {
    if (acquired) throw CalculatorError{"Display buffer already acquired"};
    const auto releaseStart = sceKernelGetProcessTime();
    std::atomic_ref label(labels[back]);
    static_assert(decltype(label)::is_always_lock_free);
    while (label.load(std::memory_order_acquire) != 0) {
        if (sceKernelGetProcessTime() - releaseStart > 5000000) throw CalculatorError{"Display buffer release timed out"};
        Wait();
    }
    acquired = true;
    return buffers[back];
}

void Display::Present() {
    if (!acquired) throw CalculatorError{"Display buffer has not been acquired"};
    std::atomic_ref label(labels[back]);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    label.store(1, std::memory_order_release);
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
    acquired = false;
}
