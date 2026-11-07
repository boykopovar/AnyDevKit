#include "Graphics/Display.hpp"
#include "Error.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"
#include "prx/libSceSystemService/libSceSystemService.hpp"
#include <atomic>

namespace {

std::size_t bufferSize(int width, int height) {
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) throw Error{"Invalid display extent"};
    const auto tiled = static_cast<std::size_t>((width + 127) / 128) * ((height + 127) / 128) * 65536;
    return (tiled + Memory::Alignment - 1) & ~(Memory::Alignment - 1);
}

}

Display::Display(int width, int height) : memory(bufferSize(width, height) * bufferCount) {
    const auto stride = memory.Size() / bufferCount;
    std::array<VideoOutBuffers, bufferCount> registered{};
    for (std::size_t index = 0; index < buffers.size(); ++index) {
        buffers[index] = memory.Allocate(stride, Memory::Alignment);
        registered[index].data = buffers[index];
    }

    handle = Check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
    try {
        constexpr std::uint64_t HighRefreshMode = 15;
        VideoOutOutputOptions options{};
        Check(sceVideoOutInitializeOutputOptions(&options), "sceVideoOutInitializeOutputOptions");
        const auto supported = Check(sceVideoOutIsOutputSupported(handle, HighRefreshMode, &options, nullptr, 0), "sceVideoOutIsOutputSupported");
        if (supported > 1) throw Error{"Invalid high refresh support result"};
        if (supported) {
            Check(sceVideoOutConfigureOutput(handle, HighRefreshMode, &options, nullptr, 0), "sceVideoOutConfigureOutput");
            VideoOutOutputStatus status{};
            Check(sceVideoOutGetOutputStatus(handle, &status), "sceVideoOutGetOutputStatus");
            if (status.refreshRate != 13) throw Error{"High refresh output mode was not applied"};
        }

        VideoOutBufferAttribute2 attribute{};
        sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 0, width, height, 0, 0, 0);
        Check(sceVideoOutRegisterBuffers2(handle, 0, 0, registered.data(), static_cast<int>(registered.size()), &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
        std::uintptr_t address = 0;
        const int count = Check(sceVideoOutGetBufferLabelAddress(handle, &address), "sceVideoOutGetBufferLabelAddress");
        if (count < static_cast<int>(bufferCount) || address == 0 || address % std::atomic_ref<std::uint64_t>::required_alignment != 0) throw Error{"Invalid display labels"};
        labels = reinterpret_cast<std::uint64_t*>(address);
    } catch (...) {
        Check(sceVideoOutClose(handle), "sceVideoOutClose");
        throw;
    }
}

Display::~Display() noexcept(false) { Check(sceVideoOutClose(handle), "sceVideoOutClose"); }

void* Display::Acquire() {
    if (acquired) throw Error{"Display buffer already acquired"};
    const auto releaseStart = sceKernelGetProcessTime();
    std::atomic_ref label(labels[back]);
    static_assert(decltype(label)::is_always_lock_free);

    while (label.load(std::memory_order_acquire) != 0) {
        if (sceKernelGetProcessTime() - releaseStart > 5000000) throw Error{"Display buffer release timed out"};
        Check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }

    acquired = true;
    return buffers[back];
}

Display::Flip Display::PrepareFlip() {
    if (!acquired) throw Error{"Display buffer has not been acquired"};
    std::atomic_ref label(labels[back]);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    label.store(1, std::memory_order_release);
    const Flip flip{handle, back, ++sequence};
    back = (back + 1) % static_cast<int>(bufferCount);
    acquired = false;

    return flip;
}

void Display::CompleteFlip() {
    if (sequence != 1) return;
    const auto start = sceKernelGetProcessTime();
    for (;;) {
        VideoOutFlipStatus status{};
        Check(sceVideoOutGetFlipStatus(handle, &status), "sceVideoOutGetFlipStatus");
        if (status.flipArg == sequence) break;
        if (sceKernelGetProcessTime() - start > 5000000) throw Error{"First display flip timed out"};
        Check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }

    Check(sceSystemServiceHideSplashScreen(), "sceSystemServiceHideSplashScreen");
}

