#include "Builder/CntBuilder.hpp"
#include "Writer/ImageWriter.hpp"
#include "Sha3.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace AnyDevKit::Pkg {
namespace {

void Put(std::span<std::byte> target, std::size_t offset, std::uint64_t value, std::size_t size) {
    Pfs::ImageWriter::Put(target, offset, value, size);
    std::reverse(target.begin() + static_cast<std::ptrdiff_t>(offset), target.begin() + static_cast<std::ptrdiff_t>(offset + size));
}

void Copy(std::span<std::byte> target, std::size_t offset, std::span<const std::byte> source) {
    if (offset > target.size() || source.size() > target.size() - offset) throw std::out_of_range("CNT copy exceeds target");
    std::copy(source.begin(), source.end(), target.begin() + static_cast<std::ptrdiff_t>(offset));
}

}

CntImage CntBuilder::Build(std::string_view contentId, const PackageKeys& keys, const PackageKeyData& keyData, const OuterImageInfo& outer, std::span<const std::byte> fih, std::span<const CntEntry> entries) {
    using Pfs::ImageWriter;
    using Crypto::Sha3;
    if (contentId.size() != 36 || fih.size() != 0x10000 || keyData.EntryKeys.size() != 2944 || keyData.ImageKey.size() != 2048 || entries.size() > 65529) throw std::invalid_argument("Invalid CNT input");
    std::vector<CntEntry> all{{0x10, "", keyData.EntryKeys}, {0x20, "", keyData.ImageKey}, {0x80, "", std::vector<std::byte>(0x1e0)}, {0x100, "", std::vector<std::byte>((entries.size() + 6) * 32)}, {1, "", std::vector<std::byte>((entries.size() + 6) * 32)}, {0x200, "", {std::byte{}}}};
    std::map<std::uint32_t, std::size_t> ids;
    std::map<std::string, std::uint32_t> names{{"", 0}};
    for (const auto& entry : entries) {
        if (entry.Id <= 0x200 || entry.Data.empty()) throw std::invalid_argument("Invalid CNT application entry");
        for (const auto c : entry.Name) if (c < 0x20 || c > 0x7e || c == '\\') throw std::invalid_argument("Invalid CNT entry name");
        if (!entry.Name.empty() && !names.emplace(entry.Name, 0).second) throw std::invalid_argument("Duplicate CNT entry name");
        all.push_back(entry);
    }
    for (std::size_t i = 0; i < all.size(); ++i) if (!ids.emplace(all[i].Id, i).second) throw std::invalid_argument("Duplicate CNT entry ID");
    for (const auto id : {0x40au, 0x1001u, 0x2000u, 0x2010u, 0x2011u}) if (!ids.contains(id)) throw std::invalid_argument("Missing required CNT entry");
    for (auto& [name, offset] : names) {
        if (name.empty()) continue;
        auto& table = all[5].Data;
        if (table.size() > 0xffffffff - name.size() - 1) throw std::length_error("CNT name table exceeds format range");
        offset = static_cast<std::uint32_t>(table.size());
        for (const auto c : name) table.push_back(static_cast<std::byte>(c));
        table.push_back(std::byte{});
    }
    CntImage result;
    std::uint64_t cursor = 0x2000;
    for (const auto& entry : all) {
        const auto end = ImageWriter::Align(ImageWriter::Add(cursor, entry.Data.size()), 16);
        if (end > 0xffff0000) throw std::length_error("CNT body exceeds 32-bit offsets");
        result.Entries.push_back({entry.Id, entry.Name, static_cast<std::uint32_t>(cursor), static_cast<std::uint32_t>(entry.Data.size())});
        cursor = end;
    }
    result.Data.resize(static_cast<std::size_t>(ImageWriter::Align(cursor, 0x10000)));
    auto& data = result.Data;
    const auto get = [&](std::uint32_t id) -> const CntEntryInfo& { return result.Entries.at(ids.at(id)); };
    const auto payload = [&](std::uint32_t id) { const auto& e = get(id); return std::span(data).subspan(e.Offset, e.Size); };
    for (std::size_t i = 0; i < all.size(); ++i) Copy(data, result.Entries[i].Offset, all[i].Data);
    auto table = payload(0x100);
    std::size_t row = 0;
    for (const auto& [id, index] : ids) {
        const auto& entry = result.Entries[index];
        const auto flags = id == 1 || id == 0x200 ? 0x40000000u : id < 0x200 ? 0x60000000u : id == 0x2000 ? 0u : 0x08000000u;
        Put(table, row, id, 4);
        Put(table, row + 4, names.at(entry.Name), 4);
        Put(table, row + 8, flags, 4);
        Put(table, row + 16, entry.Offset, 4);
        Put(table, row + 20, entry.Size, 4);
        row += 32;
    }
    Put(data, 0, 0x7f434e54, 4);
    Put(data, 4, 0x20001, 4);
    Put(data, 8, 0x80000000, 4);
    Put(data, 0xc, 12, 4);
    Put(data, 0x10, all.size(), 4);
    Put(data, 0x14, 6, 2);
    Put(data, 0x16, all.size(), 2);
    Put(data, 0x18, get(0x100).Offset, 4);
    std::uint64_t mainSize = 0;
    for (const auto id : {0x10u, 0x20u, 0x80u, 0x100u, 1u}) mainSize += get(id).Size;
    Put(data, 0x1c, mainSize, 4);
    Put(data, 0x20, 0x2000, 8);
    Put(data, 0x28, data.size() - 0x2000, 8);
    Put(data, 0x30, get(0x40a).Offset, 8);
    for (std::size_t i = 0; i < contentId.size(); ++i) data[0x40 + i] = data[0x200 + i] = static_cast<std::byte>(contentId[i]);
    Put(data, 0x74, 20, 4);
    Put(data, 0x78, 0x02020000, 4);
    Put(data, 0x7c, data.size(), 4);
    Put(data, 0x80, 0x20200722, 4);
    Put(data, 0x84, 0x01fe52e9, 4);
    Put(data, 0x400, 1, 4);
    Put(data, 0x404, 1, 4);
    Put(data, 0x408, 0xa00000000000030c, 8);
    Put(data, 0x410, 0x10000, 8);
    Put(data, 0x418, outer.Size, 8);
    const auto containerOffset = ImageWriter::Add(0x10000, outer.Size);
    const auto mountSize = ImageWriter::Add(containerOffset, data.size());
    Put(data, 0x428, mountSize, 8);
    Put(data, 0x430, mountSize, 8);
    Put(data, 0x438, 0x10000, 4);
    Copy(data, 0x440, outer.SuperblockDigest);
    Copy(data, 0x460, Sha3::Compute(fih));
    Copy(data, 0x4a0, keys.ImageSeed);
    Put(data, 0x4b0, containerOffset, 8);
    Put(data, 0x4b8, data.size(), 8);
    Put(data, 0x510, get(0x20).Offset, 4);
    Put(data, 0x514, get(0x20).Size, 4);
    Put(data, 0x518, get(0x40a).Offset, 4);
    Put(data, 0x51c, get(0x40a).Size, 4);
    Copy(data, 0x520, Sha3::Compute(payload(0x20)));
    Copy(data, 0x540, Sha3::Compute(payload(0x40a)));
    auto general = payload(0x80);
    Put(general, 0, 0xd256, 2);
    Put(general, 2, 0x102, 2);
    std::uint32_t mask = 0;
    const auto set = [&](unsigned slot, const auto& digest) { Copy(general, 0x20 + slot * 32, digest); mask |= 1u << (slot + 1); };
    Sha3 content;
    content.Update(std::span(data).subspan(0x40, 0x38));
    content.Update(outer.SuperblockDigest);
    content.Update(std::array<std::byte, 32>{});
    set(0, content.Finish());
    set(1, outer.SuperblockDigest);
    Sha3 header;
    header.Update(std::span(data).first(0x40));
    header.Update(std::span(data).subspan(0x400, 0x80));
    set(2, header.Finish());
    Sha3 media;
    bool hasMedia = false;
    for (const auto id : {0x1006u, 0x100du, 0x1200u, 0x1220u, 0x1240u, 0x1280u, 0x12a0u, 0x12c0u, 0x2040u, 0x2060u}) {
        if (!ids.contains(id)) continue;
        media.Update(Sha3::Compute(payload(id)));
        hasMedia = true;
    }
    if (hasMedia) set(3, media.Finish());
    set(5, Sha3::Compute(payload(0x2000)));
    Sha3 playgo;
    for (const auto id : {0x1001u, 0x2010u, 0x2011u}) playgo.Update(Sha3::Compute(payload(id)));
    set(6, playgo.Finish());
    set(11, outer.SuperblockDigest);
    Put(general, 0x1c, mask, 4);
    row = 0;
    for (const auto& [id, index] : ids) {
        static_cast<void>(index);
        if (id != 1) Copy(payload(1), row, Sha3::Compute(payload(id)));
        row += 32;
    }
    Copy(data, 0x140, Sha3::Compute(payload(1)));
    Copy(data, 0x160, Sha3::Compute(std::span(data).subspan(0x2000)));
    Sha3 sc1, sc2;
    for (const auto id : {0x10u, 0x20u, 0x80u, 0x100u, 1u}) {
        sc1.Update(payload(id));
        if (id != 1) sc2.Update(id == 0x100 ? payload(id).first(6 * 32) : payload(id));
    }
    Copy(data, 0x100, sc1.Finish());
    Copy(data, 0x120, sc2.Finish());
    Copy(data, 0xfe0, Sha3::Compute(std::span(data).first(0xfe0)));
    Copy(data, 0x1000, KeyBuilder::Wrap(Sha3::Compute(std::span(data).first(0x1000)), keys.PasscodeModuli[3]));
    return result;
}

}
