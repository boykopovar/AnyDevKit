#include "Builder/ProcessBuilder.hpp"
#include "Writer/BinaryWriter.hpp"

namespace AnyDevKit::Ps5 {

ProcessImage ProcessBuilder::Build(std::uint64_t address, const Ps5ElfOptions& options) {
    ProcessImage result{std::vector<std::byte>(0x2c0), {}};
    auto set = [&](std::size_t offset, std::uint64_t value, std::size_t size = 8) { BinaryWriter::Set(result.Bytes, offset, value, size); };
    auto pointer = [&](std::size_t offset, std::size_t target) { result.Relocations.push_back({BinaryWriter::Add(address, offset), 8, BinaryWriter::Add(address, target)}); };
    set(0, 0x60);
    set(8, 0x4942524f, 4);
    set(12, 5, 4);
    set(16, options.CompanionSdkVersion, 4);
    set(20, options.SdkVersion, 4);
    set(0x58, 1, 4);
    pointer(0x38, 0x60);
    pointer(0x40, 0x108);
    pointer(0x48, 0x140);
    set(0x60, 0xa8);
    set(0x68, 0x10000000e);
    set(0x108, 0x38);
    set(0x140, 0x10);
    pointer(0x90, 0x150);
    pointer(0x98, 0x1c8);
    pointer(0xc0, 0x288);
    set(0x150, 0x78);
    set(0x158, 2);
    set(0x1c8, 0xc0);
    set(0x1d0, 3);
    set(0x288, 0x38);
    set(0x290, 1);
    return result;
}

}
