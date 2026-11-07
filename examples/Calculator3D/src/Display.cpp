#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "Display.hpp"
#include "Calculator3DError.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
#include "prx/libSceSystemService/libSceSystemService.hpp"

#include <atomic>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw Calculator3DError{operation, result};
    return result;
}

}

Display::Display(int width, int height, std::size_t bufferCount) : bufferCount(bufferCount) {
    if (bufferCount < 2 || bufferCount > maximumBufferCount) throw Calculator3DError{"Invalid display buffer count"};
    constexpr std::size_t Alignment = 131072;
    const auto capacity = Canvas::StorageSize(width, height);
    const auto stride = (capacity + Alignment - 1) & ~(Alignment - 1);
    std::int64_t physicalAddress = 0;
    check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), stride * bufferCount, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
    void* address = nullptr;
    const auto result = sceKernelMapDirectMemory(&address, stride * bufferCount, 0x33, 0, physicalAddress, Alignment);
    if (result < 0) {
        check(sceKernelReleaseDirectMemory(physicalAddress, stride * bufferCount), "sceKernelReleaseDirectMemory");
        check(result, "sceKernelMapDirectMemory");
    }

    std::array<VideoOutBuffers, maximumBufferCount> registered{};
    for (std::size_t index = 0; index < bufferCount; ++index) {
        auto* pixels = reinterpret_cast<std::uint32_t*>(static_cast<std::byte*>(address) + index * stride);
        buffers[index] = std::span(pixels, capacity / sizeof(std::uint32_t));
        registered[index].data = pixels;
    }

    handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
    constexpr std::uint64_t HighRefreshMode = 15;
    VideoOutOutputOptions options{};
    check(sceVideoOutInitializeOutputOptions(&options), "sceVideoOutInitializeOutputOptions");
    const auto supported = check(sceVideoOutIsOutputSupported(handle, HighRefreshMode, &options, nullptr, 0), "sceVideoOutIsOutputSupported");
    if (supported > 1) throw Calculator3DError{"Invalid high refresh support result"};
    if (supported) {
        check(sceVideoOutConfigureOutput(handle, HighRefreshMode, &options, nullptr, 0), "sceVideoOutConfigureOutput");
        VideoOutOutputStatus status{};
        check(sceVideoOutGetOutputStatus(handle, &status), "sceVideoOutGetOutputStatus");
        if (status.refreshRate != 13) throw Calculator3DError{"High refresh output mode was not applied"};
    }

    VideoOutBufferAttribute2 attribute{};
    sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 0, width, height, 0, 0, 0);
    check(sceVideoOutRegisterBuffers2(handle, 0, 0, registered.data(), static_cast<int>(bufferCount), &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
    std::uintptr_t labelAddress = 0;
    const auto labelCount = check(sceVideoOutGetBufferLabelAddress(handle, &labelAddress), "sceVideoOutGetBufferLabelAddress");
    if (labelCount < static_cast<int>(bufferCount) || labelAddress == 0 || labelAddress % std::atomic_ref<std::uint64_t>::required_alignment != 0) throw Calculator3DError{"Invalid VideoOut buffer labels"};
    labels = reinterpret_cast<std::uint64_t*>(labelAddress);
}

void Display::Wait() { check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank"); }

std::span<std::uint32_t> Display::Acquire() {
    if (acquired) throw Calculator3DError{"Display buffer already acquired"};
    const auto releaseStart = sceKernelGetProcessTime();
    std::atomic_ref label(labels[back]);
    static_assert(decltype(label)::is_always_lock_free);

    while (label.load(std::memory_order_acquire) != 0) {
        if (sceKernelGetProcessTime() - releaseStart > 5000000) throw Calculator3DError{"Display buffer release timed out"};
        check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }

    acquired = true;
    return buffers[back];
}

Display::Flip Display::PrepareFlip() {
    if (!acquired) throw Calculator3DError{"Display buffer has not been acquired"};
    std::atomic_ref label(labels[back]);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    label.store(1, std::memory_order_release);
    const Flip flip{handle, back, ++sequence};
    back = (back + 1) % static_cast<int>(bufferCount);
    acquired = false;

    return flip;
}

void Display::Present() {
    const auto flip = PrepareFlip();
    check(sceVideoOutSubmitFlip(flip.Handle, flip.BufferIndex, 1, flip.Argument), "sceVideoOutSubmitFlip");
    CompleteFlip();
}

void Display::CompleteFlip() {
    if (sequence != 1) return;
    const auto start = sceKernelGetProcessTime();
    for (;;) {
        VideoOutFlipStatus status{};
        check(sceVideoOutGetFlipStatus(handle, &status), "sceVideoOutGetFlipStatus");
        if (status.flipArg == sequence) break;
        if (sceKernelGetProcessTime() - start > 5000000) throw Calculator3DError{"First display flip timed out"};
        check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }

    check(sceSystemServiceHideSplashScreen(), "sceSystemServiceHideSplashScreen");
}

