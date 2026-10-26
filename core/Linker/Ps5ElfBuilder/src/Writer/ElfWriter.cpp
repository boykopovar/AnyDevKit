#include "Writer/ElfWriter.hpp"
#include "Writer/BinaryWriter.hpp"
#include "Builder/ProcessBuilder.hpp"
#include "Builder/StartupBuilder.hpp"
#include "Builder/UnwindBuilder.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace AnyDevKit::Ps5 {
namespace {

std::string identifier(std::uint16_t value) {
    constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
    std::string result;
    do {
        result.insert(result.begin(), alphabet[value & 63]);
        value >>= 6;
    } while (value != 0);
    return result;
}

std::uint32_t hash(std::string_view name) {
    std::uint32_t value = 0;
    for (unsigned char c : name) {
        value = (value << 4) + c;
        const auto high = value & 0xf0000000;
        value ^= high >> 24;
        value &= ~high;
    }
    return value;
}

void validate(const ElfImage& image) {
    if (image.Read(16, 2) != 3) throw std::invalid_argument("PS5 ELF builder requires a linked position-independent ET_DYN image");
    const std::set<std::uint64_t> supported{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 20, 21, 23, 25, 26, 27, 28, 30, 32, 33, 0x6ffffef5, 0x6ffffff9, 0x6ffffffb};
    for (const auto& entry : image.DynamicEntries()) {
        if (!supported.contains(entry.Tag)) throw std::invalid_argument("Unsupported input ELF dynamic tag: " + std::to_string(entry.Tag));
        if (entry.Tag == 30 && (entry.Value & ~std::uint64_t{10}) != 0) throw std::invalid_argument("Unsupported ELF dynamic flags");
        if (entry.Tag == 0x6ffffffb && (entry.Value & ~std::uint64_t{0x8000001}) != 0) throw std::invalid_argument("Unsupported ELF dynamic flags1");
    }
    if (image.Dynamic(11) != 24) throw std::invalid_argument("Unsupported ELF symbol size");
    bool entry = false;
    for (const auto& segment : image.Segments()) {
        if (segment.FileSize != 0) image.Slice(segment.Offset, segment.FileSize);
        if (segment.Type == 1) {
            if (segment.Address % 0x4000 != segment.Offset % 0x4000 || segment.Alignment < 0x4000) throw std::invalid_argument("Link ELF with 16 KiB page alignment");
            if ((segment.Flags & 3) == 3) throw std::invalid_argument("Writable executable ELF segment");
            if ((segment.Flags & 1) != 0 && image.Read(24, 8) >= segment.Address && image.Read(24, 8) - segment.Address < segment.FileSize) entry = true;
        } else if (segment.Type != 0 && segment.Type != 2 && segment.Type != 4 && segment.Type != 6 && segment.Type != 7 && segment.Type != 0x6474e550 && segment.Type != 0x6474e551 && segment.Type != 0x6474e552) throw std::invalid_argument("Unsupported input ELF segment");
        if (segment.Type == 0x6474e551 && (segment.Flags & 1) != 0) throw std::invalid_argument("Executable stack is unsupported");
    }
    if (!entry) throw std::invalid_argument("ELF entry must point to the application's main function");
    for (const auto& first : image.Segments()) {
        if (first.Type != 1) continue;
        for (const auto& second : image.Segments()) {
            if (&first >= &second || second.Type != 1 || first.MemorySize == 0 || second.MemorySize == 0) continue;
            if (first.Address < second.Address + second.MemorySize && second.Address < first.Address + first.MemorySize) throw std::invalid_argument("Overlapping ELF load segments");
        }
    }
}

void validateRelocations(const ElfImage& image, const std::vector<Symbol>& symbols, std::vector<Relocation>& relocations) {
    for (auto& relocation : relocations) {
        const auto type = static_cast<std::uint32_t>(relocation.Info);
        const auto index = relocation.Info >> 32;
        if (index >= symbols.size()) throw std::invalid_argument("ELF relocation symbol exceeds table");
        if (type != 1 && type != 6 && type != 7 && type != 8 && type != 16 && type != 17) throw std::invalid_argument("Unsupported PS5 relocation type: " + std::to_string(type));
        bool mapped = false;
        for (const auto& segment : image.Segments()) {
            if (segment.Type == 1 && (segment.Flags & 2) != 0 && relocation.Address >= segment.Address && relocation.Address - segment.Address <= segment.MemorySize && 8 <= segment.MemorySize - (relocation.Address - segment.Address)) mapped = true;
        }
        if (!mapped) throw std::invalid_argument("ELF relocation target is not writable");
        if (type == 8 && index != 0) throw std::invalid_argument("Relative relocation has a symbol");
        if (index != 0 && symbols[index].Section != 0 && (type == 1 || type == 6 || type == 7)) {
            if (symbols[index].Section >= 0xff00 || (symbols[index].Info & 15) == 6 || (symbols[index].Info & 15) == 10) throw std::invalid_argument("Unsupported defined relocation symbol");
            relocation.Info = 8;
            relocation.Addend = symbols[index].Value + (type == 1 ? relocation.Addend : 0);
        }
    }
}

}

std::vector<std::byte> ElfWriter::Write(std::span<const std::byte> input, const ImportCatalog& imports, const Ps5ElfOptions& options) {
    const ElfImage image(input);
    validate(image);
    auto symbols = image.Symbols();
    auto relocations = image.Relocations(7, 8);
    auto plt = image.Relocations(23, 2);
    validateRelocations(image, symbols, relocations);
    validateRelocations(image, symbols, plt);
    std::uint64_t end = 0;
    for (const auto& segment : image.Segments()) if (segment.Type == 1) end = std::max(end, BinaryWriter::Add(segment.Address, segment.MemorySize));
    const auto codeAddress = BinaryWriter::Align(end, 0x4000);
    const auto processAddress = BinaryWriter::Add(codeAddress, 0x4000);
    auto process = ProcessBuilder::Build(processAddress, options);
    const auto slots = BinaryWriter::Add(processAddress, process.Bytes.size());
    process.Bytes.resize(process.Bytes.size() + 24);
    auto startup = StartupBuilder::Build(image, codeAddress, slots);
    if (startup.Bytes.size() > 0x4000) throw std::length_error("PS5 startup exceeds its segment");
    relocations.insert(relocations.end(), process.Relocations.begin(), process.Relocations.end());
    const std::array<std::string, 3> startupNames{"_init_env", "atexit", "exit"};
    for (std::size_t i = 0; i < startupNames.size(); ++i) {
        auto found = std::ranges::find(symbols, startupNames[i], &Symbol::Name);
        std::size_t index;
        if (found == symbols.end()) {
            index = symbols.size();
            symbols.push_back({startupNames[i], 0x12, 0, 0, 0, 0});
        } else {
            index = static_cast<std::size_t>(found - symbols.begin());
            if (found->Section != 0) throw std::invalid_argument("Application overrides a required PS5 startup export");
        }
        relocations.push_back({slots + i * 8, (static_cast<std::uint64_t>(index) << 32) | 6, 0});
    }
    std::map<std::string, std::uint16_t> moduleIds;
    std::map<std::pair<std::string, std::string>, std::uint16_t> libraryIds;
    std::map<std::string, Ps5Import> modules;
    std::map<std::pair<std::string, std::string>, Ps5Import> libraries;
    for (std::size_t i = 1; i < symbols.size(); ++i) {
        auto& symbol = symbols[i];
        if (symbol.Section != 0) continue;
        const auto imported = imports.Resolve(symbol.Name);
        if ((symbol.Info & 15) != 0 && (symbol.Info & 15) != imported.Type) throw std::invalid_argument("PS5 import type mismatch: " + symbol.Name);
        if (!moduleIds.contains(imported.Module)) {
            if (moduleIds.size() >= 65535) throw std::length_error("Too many PS5 import modules");
            moduleIds.emplace(imported.Module, static_cast<std::uint16_t>(moduleIds.size() + 1));
            modules.emplace(imported.Module, imported);
        }
        const auto key = std::pair{imported.Module, imported.Library};
        if (!libraryIds.contains(key)) {
            if (libraryIds.size() >= 65535) throw std::length_error("Too many PS5 import libraries");
            libraryIds.emplace(key, static_cast<std::uint16_t>(libraryIds.size()));
            libraries.emplace(key, imported);
        }
        symbol.Name = imported.Nid + "#" + identifier(libraryIds.at(key)) + "#" + identifier(moduleIds.at(imported.Module));
        symbol.Info = static_cast<std::uint8_t>(0x10 | imported.Type);
        symbol.Visibility = 0;
    }
    std::vector<std::byte> strings{std::byte{0}};
    std::vector<std::byte> symbolBytes;
    for (const auto& symbol : symbols) {
        BinaryWriter::Append(symbolBytes, symbol.Name.empty() ? 0 : BinaryWriter::String(strings, symbol.Name), 4);
        BinaryWriter::Append(symbolBytes, symbol.Info, 1);
        BinaryWriter::Append(symbolBytes, symbol.Visibility, 1);
        BinaryWriter::Append(symbolBytes, symbol.Section == 0 ? 0 : 1, 2);
        BinaryWriter::Append(symbolBytes, symbol.Value, 8);
        BinaryWriter::Append(symbolBytes, symbol.Size, 8);
    }
    std::vector<DynamicEntry> dynamic;
    for (const auto& [module, imported] : modules) {
        dynamic.push_back({1, BinaryWriter::String(strings, imported.FileName)});
        dynamic.push_back({0x61000045, (static_cast<std::uint64_t>(moduleIds.at(module)) << 48) | (static_cast<std::uint64_t>(imported.ModuleVersion) << 32) | BinaryWriter::String(strings, module)});
    }
    for (const auto& [key, imported] : libraries) {
        const auto id = static_cast<std::uint64_t>(libraryIds.at(key)) << 48;
        dynamic.push_back({0x61000049, id | (static_cast<std::uint64_t>(imported.LibraryVersion) << 32) | BinaryWriter::String(strings, imported.Library)});
        dynamic.push_back({0x61000019, id | 9});
    }
    dynamic.push_back({0x61000043, 0x10100000000ull | BinaryWriter::String(strings, options.ModuleName)});
    dynamic.push_back({0x61000011, 0});
    dynamic.push_back({0x61000041, BinaryWriter::String(strings, options.FileName)});
    std::vector<std::byte> metadata;
    const auto metadataAddress = BinaryWriter::Add(processAddress, 0x4000);
    auto append = [&](std::span<const std::byte> data) {
        metadata.resize(static_cast<std::size_t>(BinaryWriter::Align(metadata.size(), 8)));
        const auto address = BinaryWriter::Add(metadataAddress, metadata.size());
        metadata.insert(metadata.end(), data.begin(), data.end());
        return address;
    };
    const auto stringAddress = append(strings);
    const auto symbolAddress = append(symbolBytes);
    auto appendRelocations = [&](const std::vector<Relocation>& values) {
        std::vector<std::byte> data;
        for (const auto& relocation : values) {
            BinaryWriter::Append(data, relocation.Address, 8);
            BinaryWriter::Append(data, relocation.Info, 8);
            BinaryWriter::Append(data, relocation.Addend, 8);
        }
        return append(data);
    };
    const auto relaAddress = appendRelocations(relocations);
    const auto pltAddress = appendRelocations(plt);
    if (symbols.size() > 0xffffffff) throw std::length_error("Too many ELF symbols");
    const auto bucketCount = static_cast<std::uint32_t>(std::max<std::size_t>(1, symbols.size() / 2));
    std::vector<std::uint32_t> buckets(bucketCount);
    std::vector<std::uint32_t> chains(symbols.size());
    for (std::size_t i = 1; i < symbols.size(); ++i) {
        const auto bucket = hash(symbols[i].Name) % bucketCount;
        chains[i] = buckets[bucket];
        buckets[bucket] = static_cast<std::uint32_t>(i);
    }
    std::vector<std::byte> hashBytes;
    BinaryWriter::Append(hashBytes, bucketCount, 4);
    BinaryWriter::Append(hashBytes, symbols.size(), 4);
    for (auto value : buckets) BinaryWriter::Append(hashBytes, value, 4);
    for (auto value : chains) BinaryWriter::Append(hashBytes, value, 4);
    const auto hashAddress = append(hashBytes);
    metadata.resize(static_cast<std::size_t>(BinaryWriter::Align(metadata.size(), 8)));
    const auto unwindAddress = BinaryWriter::Add(metadataAddress, metadata.size());
    const auto unwind = UnwindBuilder::Build(image, startup, codeAddress, unwindAddress);
    append(unwind.Bytes);
    dynamic.insert(dynamic.end(), {{4, hashAddress}, {5, stringAddress}, {6, symbolAddress}, {7, relaAddress}, {8, relocations.size() * 24}, {9, 24}, {10, strings.size()}, {11, 24}, {0x6100003d, hashBytes.size()}, {0x6100003f, symbolBytes.size()}, {21, 0}});
    if (!plt.empty()) dynamic.insert(dynamic.end(), {{23, pltAddress}, {2, plt.size() * 24}, {20, 7}, {3, image.Dynamic(3)}});
    metadata.resize(static_cast<std::size_t>(BinaryWriter::Align(metadata.size(), 8)));
    const auto dynamicAddress = BinaryWriter::Add(metadataAddress, metadata.size());
    for (const auto& entry : dynamic) {
        BinaryWriter::Append(metadata, entry.Tag, 8);
        BinaryWriter::Append(metadata, entry.Value, 8);
    }
    BinaryWriter::Append(metadata, 0, 8);
    BinaryWriter::Append(metadata, 0, 8);
    const auto dynamicSize = (dynamic.size() + 1) * 16;
    std::vector<std::byte> output(0x4000);
    output.insert(output.end(), input.begin(), input.end());
    std::vector<Segment> segments;
    for (auto segment : image.Segments()) {
        if (segment.Type == 0 || segment.Type == 2 || segment.Type == 6 || segment.Type == 4 || segment.Type == 0x6474e550) continue;
        segment.Offset = BinaryWriter::Add(segment.Offset, 0x4000);
        segments.push_back(segment);
    }
    auto addSegment = [&](std::uint32_t flags, std::uint64_t address, std::span<const std::byte> data) {
        output.resize(static_cast<std::size_t>(BinaryWriter::Align(output.size(), 0x4000)));
        const auto offset = output.size();
        output.insert(output.end(), data.begin(), data.end());
        segments.push_back({1, flags, offset, address, data.size(), data.size(), 0x4000});
        return offset;
    };
    addSegment(5, codeAddress, startup.Bytes);
    const auto processOffset = addSegment(6, processAddress, process.Bytes);
    const auto metadataOffset = addSegment(4, metadataAddress, metadata);
    segments.push_back({0x61000001, 4, processOffset, processAddress, 0x60, 0x60, 8});
    segments.push_back({2, 4, metadataOffset + dynamicAddress - metadataAddress, dynamicAddress, dynamicSize, dynamicSize, 8});
    segments.push_back({0x6474e550, 4, metadataOffset + unwindAddress - metadataAddress + unwind.HeaderOffset, unwindAddress + unwind.HeaderOffset, unwind.HeaderSize, unwind.HeaderSize, 4});
    if (64 + segments.size() * 56 > 0x4000) throw std::length_error("Too many ELF program headers");
    BinaryWriter::Set(output, 0, 0x464c457f, 4);
    BinaryWriter::Set(output, 4, 0x0209010102, 5);
    BinaryWriter::Set(output, 16, 0xfe10, 2);
    BinaryWriter::Set(output, 18, 62, 2);
    BinaryWriter::Set(output, 20, 1, 4);
    BinaryWriter::Set(output, 24, startup.Entry, 8);
    BinaryWriter::Set(output, 32, 64, 8);
    BinaryWriter::Set(output, 52, 64, 2);
    BinaryWriter::Set(output, 54, 56, 2);
    BinaryWriter::Set(output, 56, segments.size(), 2);
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto& segment = segments[i];
        const auto offset = 64 + i * 56;
        BinaryWriter::Set(output, offset, segment.Type, 4);
        BinaryWriter::Set(output, offset + 4, segment.Flags, 4);
        BinaryWriter::Set(output, offset + 8, segment.Offset, 8);
        BinaryWriter::Set(output, offset + 16, segment.Address, 8);
        BinaryWriter::Set(output, offset + 32, segment.FileSize, 8);
        BinaryWriter::Set(output, offset + 40, segment.MemorySize, 8);
        BinaryWriter::Set(output, offset + 48, segment.Alignment, 8);
    }
    return output;
}

}
