#include "LibcModuleBuilder.hpp"
#include "Runtime/Initialization.hpp"
#include "Writer/BinaryWriter.hpp"

#include <nid/NidCompute.hpp>
#include <array>
#include <algorithm>
#include <stdexcept>

namespace AnyDevKit {

std::vector<std::byte> LibcModuleBuilder::Build(const ImportCatalog& imports, const Ps5ElfOptions& options) {
    using Ps5::BinaryWriter;
    if (options.SdkVersion == 0 || options.CompanionSdkVersion == 0) throw std::invalid_argument("Libc module requires SDK versions");
    const auto allocator = imports.Resolve("_malloc_init");
    if (allocator.Module != "libSceLibcInternal" || allocator.FileName != "libSceLibcInternal.sprx" || allocator.Type != 2) throw std::invalid_argument("Libc initialization requires the system allocator");
    constexpr std::uint64_t codeAddress = 0x4000;
    constexpr std::uint64_t dataAddress = 0x8000;
    constexpr std::uint64_t metadataAddress = 0xc000;
    auto code = Libc::Initialization::Build(codeAddress, dataAddress);
    const auto fini = codeAddress + code.size();
    for (auto value : {0x31, 0xc0, 0xc3}) code.push_back(static_cast<std::byte>(value));
    std::vector<std::byte> data(0x50);
    BinaryWriter::Set(data, 8, 0x20, 8);
    BinaryWriter::Set(data, 16, 0x3c13f4bf, 4);
    BinaryWriter::Set(data, 20, 3, 4);
    BinaryWriter::Set(data, 24, options.CompanionSdkVersion, 4);
    BinaryWriter::Set(data, 28, options.SdkVersion, 4);
    BinaryWriter::Set(data, 32, 1, 4);
    BinaryWriter::Set(data, 40, 1, 4);
    std::vector<std::byte> strings{std::byte{0}};
    const auto fileName = BinaryWriter::String(strings, "libc.prx");
    const auto moduleName = BinaryWriter::String(strings, "libc");
    const auto dependency = BinaryWriter::String(strings, allocator.FileName);
    const auto importModule = BinaryWriter::String(strings, allocator.Module);
    const auto importLibrary = BinaryWriter::String(strings, allocator.Library);
    const auto initName = BinaryWriter::String(strings, allocator.Nid + "#B#B");
    const auto markerName = BinaryWriter::String(strings, Nid::ComputeNid("Need_sceLibc", "") + "#A#A");
    std::vector<std::byte> metadata;
    auto append = [&](std::span<const std::byte> bytes) {
        metadata.resize(static_cast<std::size_t>(BinaryWriter::Align(metadata.size(), 8)));
        const auto address = metadataAddress + metadata.size();
        metadata.insert(metadata.end(), bytes.begin(), bytes.end());
        return address;
    };
    const auto stringAddress = append(strings);
    std::vector<std::byte> symbols(72);
    BinaryWriter::Set(symbols, 24, initName, 4);
    BinaryWriter::Set(symbols, 28, 0x12, 1);
    BinaryWriter::Set(symbols, 48, markerName, 4);
    BinaryWriter::Set(symbols, 52, 0x11, 1);
    BinaryWriter::Set(symbols, 54, 1, 2);
    BinaryWriter::Set(symbols, 56, dataAddress + 40, 8);
    BinaryWriter::Set(symbols, 64, 4, 8);
    const auto symbolAddress = append(symbols);
    std::vector<std::byte> relocations;
    BinaryWriter::Append(relocations, dataAddress, 8);
    BinaryWriter::Append(relocations, (std::uint64_t{1} << 32) | 6, 8);
    BinaryWriter::Append(relocations, 0, 8);
    const auto relocationAddress = append(relocations);
    std::vector<std::byte> hash;
    for (auto value : {1, 3, 2, 0, 0, 1}) BinaryWriter::Append(hash, value, 4);
    const auto hashAddress = append(hash);
    const std::array<std::pair<std::uint64_t, std::uint64_t>, 26> dynamic{{
        {1, dependency}, {0x61000043, (std::uint64_t{0x101} << 32) | moduleName},
        {0x61000045, (std::uint64_t{1} << 48) | (std::uint64_t{allocator.ModuleVersion} << 32) | importModule},
        {0x61000047, (std::uint64_t{1} << 32) | moduleName}, {0x61000017, 0},
        {0x61000049, (std::uint64_t{1} << 48) | (std::uint64_t{allocator.LibraryVersion} << 32) | importLibrary},
        {0x61000019, (std::uint64_t{1} << 48) | 9}, {0x61000041, fileName}, {0x61000011, 0},
        {5, stringAddress}, {10, strings.size()}, {6, symbolAddress}, {11, 24}, {4, hashAddress},
        {3, dataAddress + 48}, {23, relocationAddress}, {2, 0}, {20, 7},
        {7, relocationAddress}, {8, relocations.size()}, {9, 24}, {12, codeAddress}, {13, fini},
        {0x6100003f, symbols.size()}, {0x6100003d, hash.size()}, {0, 0}
    }};
    std::vector<std::byte> dynamicBytes;
    for (const auto& [tag, value] : dynamic) {
        BinaryWriter::Append(dynamicBytes, tag, 8);
        BinaryWriter::Append(dynamicBytes, value, 8);
    }
    const auto dynamicAddress = append(dynamicBytes);
    std::vector<std::byte> output(metadataAddress + metadata.size());
    std::ranges::copy(code, output.begin() + codeAddress);
    std::ranges::copy(data, output.begin() + dataAddress);
    std::ranges::copy(metadata, output.begin() + metadataAddress);
    BinaryWriter::Set(output, 0, 0x464c457f, 4);
    BinaryWriter::Set(output, 4, 0x0209010102, 5);
    BinaryWriter::Set(output, 16, 0xfe18, 2);
    BinaryWriter::Set(output, 18, 62, 2);
    BinaryWriter::Set(output, 20, 1, 4);
    BinaryWriter::Set(output, 24, codeAddress, 8);
    BinaryWriter::Set(output, 32, 64, 8);
    BinaryWriter::Set(output, 52, 64, 2);
    BinaryWriter::Set(output, 54, 56, 2);
    BinaryWriter::Set(output, 56, 5, 2);
    auto segment = [&](std::size_t index, std::uint32_t type, std::uint32_t flags, std::uint64_t address, std::uint64_t size, std::uint64_t alignment) {
        const auto offset = 64 + index * 56;
        BinaryWriter::Set(output, offset, type, 4);
        BinaryWriter::Set(output, offset + 4, flags, 4);
        for (auto field : {8, 16, 24}) BinaryWriter::Set(output, offset + field, address, 8);
        BinaryWriter::Set(output, offset + 32, size, 8);
        BinaryWriter::Set(output, offset + 40, size, 8);
        BinaryWriter::Set(output, offset + 48, alignment, 8);
    };
    segment(0, 1, 1, codeAddress, code.size(), 0x4000);
    segment(1, 1, 6, dataAddress, data.size(), 0x4000);
    segment(2, 1, 0, metadataAddress, metadata.size(), 0x4000);
    segment(3, 0x61000002, 4, dataAddress + 8, 0x20, 8);
    segment(4, 2, 6, dynamicAddress, dynamicBytes.size(), 8);
    return output;
}

}
