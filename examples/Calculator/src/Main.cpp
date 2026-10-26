#include "Canvas.hpp"
#include "prx/libSceVideoOut/libSceVideoOut.hpp"

#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(result));
    return result;
}

}

int main() {
    try {
        constexpr int Width = 480;
        constexpr int Height = 704;
        constexpr std::size_t Alignment = 65536;
        std::vector<std::uint32_t> storage(Width * Height + Alignment / sizeof(std::uint32_t));
        const auto address = reinterpret_cast<std::uintptr_t>(storage.data());
        const auto offset = ((Alignment - address % Alignment) % Alignment) / sizeof(std::uint32_t);
        auto pixels = std::span(storage).subspan(offset, Width * Height);
        Canvas(pixels, Width, Height).DrawCalculator();
        const int handle = check(sceVideoOutOpen(255, 0, 0, nullptr), "sceVideoOutOpen");
        VideoOutBufferAttribute2 attribute{};
        sceVideoOutSetBufferAttribute2(&attribute, 0x8000000000000000ull, 1, Width, Height, 0, 0, 0);
        VideoOutBuffers buffer{pixels.data(), nullptr, {nullptr, nullptr}};
        check(sceVideoOutRegisterBuffers2(handle, 0, 0, &buffer, 1, &attribute, 0, nullptr), "sceVideoOutRegisterBuffers2");
        check(sceVideoOutSubmitFlip(handle, 0, 1, 1), "sceVideoOutSubmitFlip");
        for (;;) check(sceVideoOutWaitVblank(handle), "sceVideoOutWaitVblank");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Calculator: %s\n", error.what());
        return 1;
    }
}
