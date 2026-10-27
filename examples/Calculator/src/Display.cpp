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
    std::uintptr_t labelAddress = 0;
    const auto labelCount = check(sceVideoOutGetBufferLabelAddress(handle, &labelAddress), "sceVideoOutGetBufferLabelAddress");
    if (labelCount < static_cast<int>(buffers.size()) || labelAddress == 0 || labelAddress % std::atomic_ref<std::uint64_t>::required_alignment != 0) throw CalculatorError{"Invalid VideoOut buffer labels"};
    labels = reinterpret_cast<std::uint64_t*>(labelAddress);
    void* storage = nullptr;
    check(sceKernelMapFlexibleMemory(&storage, capacity, 3, 0), "sceKernelMapFlexibleMemory");
    frame = std::span(static_cast<std::uint32_t*>(storage), capacity / sizeof(std::uint32_t));
    memset_nid_postfix(frame.data(), 0, frame.size_bytes());
}

Display::~Display() noexcept(false) { check(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(frame.data()), frame.size_bytes()), "sceKernelMunmap"); }
Canvas Display::BackBuffer() { return Canvas(frame, width, height); }
void Display::Wait() { check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank"); }

void Display::Present() {
    const auto releaseStart = sceKernelGetProcessTime();
    std::atomic_ref label(labels[back]);
    static_assert(decltype(label)::is_always_lock_free);
    while (label.load(std::memory_order_acquire) != 0) {
        if (sceKernelGetProcessTime() - releaseStart > 5000000) throw CalculatorError{"Display buffer release timed out"};
        Wait();
    }
    memcpy_nid_postfix(buffers[back].data(), frame.data(), frame.size_bytes());
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
}
