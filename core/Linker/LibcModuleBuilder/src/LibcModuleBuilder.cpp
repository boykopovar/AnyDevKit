#include "LibcModuleBuilder.hpp"
#include "Runtime/Initialization.hpp"
#include "Sha256.hpp"
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
    if (allocator.Module != "libSceLibcInternal" || allocator.FileName != "libSceLibcInternal.sprx") throw std::invalid_argument("Libc initialization requires the system allocator");
    constexpr std::uint64_t codeAddress = 0x10;
    constexpr std::uint64_t dataAddress = 0x4000;
    constexpr std::uint64_t writableAddress = 0x8000;
    constexpr std::uint64_t metadataAddress = 0xc000;
    auto code = Libc::Initialization::Build(codeAddress, dataAddress + 80);
    const auto fini = codeAddress + code.size();
    for (auto value : {0x31, 0xc0, 0xc3}) code.push_back(static_cast<std::byte>(value));
    std::vector<std::byte> data(0x58);
    BinaryWriter::Set(data, 8, 0x20, 8);
    BinaryWriter::Set(data, 16, 0x3c13f4bf, 4);
    BinaryWriter::Set(data, 20, 3, 4);
    BinaryWriter::Set(data, 24, options.CompanionSdkVersion, 4);
    BinaryWriter::Set(data, 28, options.SdkVersion, 4);
    BinaryWriter::Set(data, 32, 1, 4);
    std::vector<std::byte> writable(4);
    BinaryWriter::Set(writable, 0, 1, 4);
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
    BinaryWriter::Set(symbols, 56, writableAddress, 8);
    BinaryWriter::Set(symbols, 64, 4, 8);
    const auto symbolAddress = append(symbols);
    std::vector<std::byte> relocations;
    BinaryWriter::Append(relocations, dataAddress + 72, 8);
    BinaryWriter::Append(relocations, 8, 8);
    BinaryWriter::Append(relocations, fini, 8);
    std::vector<std::byte> plt;
    BinaryWriter::Append(plt, dataAddress + 80, 8);
    BinaryWriter::Append(plt, (std::uint64_t{1} << 32) | 7, 8);
    BinaryWriter::Append(plt, 0, 8);
    const auto pltAddress = append(plt);
    const auto relocationAddress = append(relocations);
    const std::array<std::string, 3> hashNames{"", allocator.Nid + "#" + allocator.Library + "#" + allocator.Module, Nid::ComputeNid("Need_sceLibc", "") + "#libc#libc"};
    const auto hash = BinaryWriter::HashTable(hashNames);
    const auto hashAddress = append(hash);
    std::vector<std::byte> note;
    for (auto value : {4, 20, 3, 0x554e47}) BinaryWriter::Append(note, value, 4);
    const auto digest = Crypto::Sha256::Compute(code);
    note.insert(note.end(), digest.begin(), digest.begin() + 20);
    const auto noteAddress = append(note);
    const std::array<std::pair<std::uint64_t, std::uint64_t>, 34> dynamic{{
        {1, dependency}, {14, fileName}, {0x61000043, (std::uint64_t{0x101} << 32) | moduleName},
        {0x61000045, (std::uint64_t{1} << 48) | (std::uint64_t{allocator.ModuleVersion} << 32) | importModule},
        {0x61000047, (std::uint64_t{1} << 32) | moduleName}, {0x61000017, 1},
        {0x61000049, (std::uint64_t{1} << 48) | (std::uint64_t{allocator.LibraryVersion} << 32) | importLibrary},
        {0x61000019, (std::uint64_t{1} << 48) | 9}, {0x61000041, fileName}, {0x61000011, 0},
        {5, stringAddress}, {10, strings.size()}, {6, symbolAddress}, {11, 24}, {4, hashAddress},
        {3, dataAddress + 48}, {23, pltAddress}, {2, plt.size()}, {20, 7},
        {7, relocationAddress}, {8, relocations.size()}, {9, 24}, {0x6ffffff9, 1}, {12, codeAddress}, {13, fini},
        {32, dataAddress + 72}, {33, 8}, {25, 0}, {27, 0}, {26, 0}, {28, 0},
        {0x6100003f, symbols.size()}, {0x6100003d, hash.size()}, {0, 0}
    }};
    std::vector<std::byte> dynamicBytes;
    for (const auto& [tag, value] : dynamic) {
        BinaryWriter::Append(dynamicBytes, tag, 8);
        BinaryWriter::Append(dynamicBytes, value, 8);
    }
    const auto dynamicAddress = append(dynamicBytes);
    std::vector<std::byte> output(0x4000 + metadataAddress + metadata.size());
    std::ranges::copy(code, output.begin() + 0x4000 + codeAddress);
    std::ranges::copy(data, output.begin() + 0x4000 + dataAddress);
    std::ranges::copy(writable, output.begin() + 0x4000 + writableAddress);
    std::ranges::copy(metadata, output.begin() + 0x4000 + metadataAddress);
    BinaryWriter::Set(output, 0, 0x464c457f, 4);
    BinaryWriter::Set(output, 4, 0x0209010102, 5);
    BinaryWriter::Set(output, 16, 0xfe18, 2);
    BinaryWriter::Set(output, 18, 62, 2);
    BinaryWriter::Set(output, 20, 1, 4);
    BinaryWriter::Set(output, 24, 0, 8);
    BinaryWriter::Set(output, 32, 64, 8);
    BinaryWriter::Set(output, 52, 64, 2);
    BinaryWriter::Set(output, 54, 56, 2);
    BinaryWriter::Set(output, 56, 8, 2);
    auto segment = [&](std::size_t index, std::uint32_t type, std::uint32_t flags, std::uint64_t address, std::uint64_t size, std::uint64_t alignment) {
        const auto offset = 64 + index * 56;
        BinaryWriter::Set(output, offset, type, 4);
        BinaryWriter::Set(output, offset + 4, flags, 4);
        BinaryWriter::Set(output, offset + 8, 0x4000 + address, 8);
        for (auto field : {16, 24}) BinaryWriter::Set(output, offset + field, address, 8);
        BinaryWriter::Set(output, offset + 32, size, 8);
        BinaryWriter::Set(output, offset + 40, size, 8);
        BinaryWriter::Set(output, offset + 48, alignment, 8);
    };
    segment(0, 1, 1, 0, codeAddress + code.size(), 0x4000);
    segment(1, 1, 6, dataAddress, data.size(), 0x4000);
    segment(2, 1, 6, writableAddress, writable.size(), 0x4000);
    segment(3, 1, 0, metadataAddress, metadata.size(), 0x4000);
    segment(4, 0x61000002, 4, dataAddress + 8, 0x20, 8);
    segment(5, 2, 6, dynamicAddress, dynamicBytes.size(), 8);
    segment(6, 4, 0, noteAddress, note.size(), 4);
    segment(7, 0x6474e552, 4, dataAddress, data.size(), 1);
    BinaryWriter::Set(output, 64 + 56 + 40, 0x4000, 8);
    BinaryWriter::Set(output, 64 + 7 * 56 + 40, 0x4000, 8);
    return output;
}

}
