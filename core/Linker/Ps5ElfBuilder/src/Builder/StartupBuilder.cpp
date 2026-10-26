#include "Builder/StartupBuilder.hpp"
#include "Writer/BinaryWriter.hpp"

#include <bit>
#include <initializer_list>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::Ps5 {

StartupImage StartupBuilder::Build(const ElfImage& image, std::uint64_t codeAddress, std::uint64_t importSlots) {
    StartupImage result{{}, 0};
    auto bytes = [&](std::initializer_list<unsigned char> values) { for (auto value : values) result.Bytes.push_back(static_cast<std::byte>(value)); };
    auto relative = [&](std::uint64_t target) {
        const auto next = BinaryWriter::Add(codeAddress, result.Bytes.size() + 4);
        if ((target >= next && target - next > 0x7fffffff) || (target < next && next - target > 0x80000000)) throw std::invalid_argument("PS5 startup target exceeds RIP-relative range");
        BinaryWriter::Append(result.Bytes, static_cast<std::uint32_t>(target - next), 4);
    };
    auto direct = [&](std::uint64_t target) { bytes({0xe8}); relative(target); };
    auto indirect = [&](std::uint64_t target) { bytes({0xff, 0x15}); relative(target); };
    auto array = [&](std::uint64_t addressTag, std::uint64_t sizeTag, bool reverse) {
        const auto size = image.Dynamic(sizeTag, false);
        if (size == 0) return;
        if (size % 8 != 0) throw std::invalid_argument("Malformed ELF constructor array");
        const auto address = image.Dynamic(addressTag);
        image.Offset(address, size);
        for (std::uint64_t i = 0; i < size; i += 8) indirect(BinaryWriter::Add(address, reverse ? size - 8 - i : i));
    };
    bytes({0x48, 0x83, 0xec, 0x08});
    array(26, 28, true);
    const auto fini = image.Dynamic(13, false);
    if (fini != 0) direct(fini);
    bytes({0x48, 0x83, 0xc4, 0x08, 0xc3});
    result.Entry = BinaryWriter::Add(codeAddress, result.Bytes.size());
    bytes({0x55, 0x48, 0x89, 0xe5, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x48, 0x83, 0xec, 0x08});
    bytes({0x44, 0x8b, 0x27, 0x4c, 0x8d, 0x6f, 0x08, 0x49, 0x89, 0xf6});
    indirect(importSlots);
    bytes({0x4c, 0x89, 0xf7});
    indirect(importSlots + 8);
    bytes({0x85, 0xc0, 0x74, 0x02, 0x0f, 0x0b});
    bytes({0x48, 0x8d, 0x3d});
    relative(codeAddress);
    indirect(importSlots + 8);
    bytes({0x85, 0xc0, 0x74, 0x02, 0x0f, 0x0b});
    array(32, 33, false);
    const auto init = image.Dynamic(12, false);
    if (init != 0) direct(init);
    array(25, 27, false);
    bytes({0x44, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0x31, 0xd2});
    direct(image.Read(24, 8));
    bytes({0x89, 0xc7});
    indirect(importSlots + 16);
    bytes({0x0f, 0x0b});
    return result;
}

}
