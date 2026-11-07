#include "WindowsStartupBuilder.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <string_view>

namespace AnyDevKit {

std::vector<std::byte> WindowsStartupBuilder::Build() {
    constexpr std::array<std::uint8_t, 48> stackProbe{
        0x51, 0x50, 0x48, 0x8d, 0x4c, 0x24, 0x18, 0x48, 0x3d, 0x00, 0x10, 0x00, 0x00, 0x72, 0x18, 0x48,
        0x81, 0xe9, 0x00, 0x10, 0x00, 0x00, 0xf6, 0x01, 0x00, 0x48, 0x2d, 0x00, 0x10, 0x00, 0x00, 0x48,
        0x3d, 0x00, 0x10, 0x00, 0x00, 0x73, 0xe8, 0x48, 0x29, 0xc1, 0xf6, 0x01, 0x00, 0x58, 0x59, 0xc3
    };
    constexpr std::size_t relocationsOffset = 70 + stackProbe.size();
    constexpr std::size_t symbolsOffset = relocationsOffset + 20;
    constexpr std::size_t stringsOffset = symbolsOffset + 5 * 18;
    std::vector<std::byte> result(stringsOffset + 4);
    const auto put = [&](std::size_t offset, std::uint32_t value, unsigned size) {
        for (unsigned i = 0; i < size; ++i) result.at(offset + i) = static_cast<std::byte>((value >> (8 * i)) & 0xff);
    };
    put(0, 0x8664, 2);
    put(2, 1, 2);
    put(8, symbolsOffset, 4);
    put(12, 5, 4);
    constexpr std::string_view section = ".text";

    for (std::size_t i = 0; i < section.size(); ++i) result[20 + i] = static_cast<std::byte>(section[i]);
    put(36, 10 + stackProbe.size(), 4);
    put(40, 60, 4);
    put(44, relocationsOffset, 4);
    put(52, 2, 2);
    put(56, 0x60500020, 4);

    for (unsigned i = 0; i < 2; ++i) {
        result[60 + i * 5] = std::byte{0xe9};
        put(relocationsOffset + i * 10, 1 + i * 5, 4);
        put(relocationsOffset + 4 + i * 10, 1 + i * 2, 4);
        put(relocationsOffset + 8 + i * 10, 4, 2);
    }

    for (std::size_t i = 0; i < stackProbe.size(); ++i) result[70 + i] = static_cast<std::byte>(stackProbe[i]);
    constexpr std::array<std::string_view, 5> names{"AnyDevKitInvokeMain", "main", "atexit", "AnyDevKitRegisterDestructor", "___chkstk_ms"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto offset = symbolsOffset + i * 18;
        put(offset + 4, static_cast<std::uint32_t>(result.size() - stringsOffset), 4);

        for (const auto c : names[i]) result.push_back(static_cast<std::byte>(c));
        result.push_back(std::byte{});
        put(offset + 8, i == 4 ? 10 : i == 2 ? 5 : 0, 4);
        put(offset + 12, i % 2 == 0 ? 1 : 0, 2);
        put(offset + 14, 0x20, 2);
        put(offset + 16, 2, 1);
    }

    put(stringsOffset, static_cast<std::uint32_t>(result.size() - stringsOffset), 4);
    return result;
}

void WindowsStartupBuilder::WriteFile(const std::filesystem::path& output) {
    const auto bytes = Build();
    std::ofstream stream;
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    stream.open(output, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.close();
}

}

