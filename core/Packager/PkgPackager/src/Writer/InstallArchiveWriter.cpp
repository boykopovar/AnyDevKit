#include "Writer/InstallArchiveWriter.hpp"
#include "Crc32.hpp"
#include "Writer/ImageWriter.hpp"

#include <array>
#include <limits>
#include <set>
#include <stdexcept>

namespace AnyDevKit::Pkg {

std::vector<std::byte> InstallArchiveWriter::Build(std::span<const InstallArchiveEntry> entries) {
    if (entries.empty() || entries.size() >= 0xffff) throw std::invalid_argument("Invalid SI entry count");
    std::set<std::string> paths;
    std::uint64_t total = 22;
    for (const auto& entry : entries) {
        if (entry.Path.empty() || entry.Path.size() > 0xffff || entry.Path.front() == '/' || entry.Path.back() == '/') throw std::invalid_argument("Invalid SI entry path");
        std::size_t start = 0;
        for (std::size_t i = 0; i <= entry.Path.size(); ++i) {
            if (i == entry.Path.size() || entry.Path[i] == '/') {
                const auto component = entry.Path.substr(start, i - start);
                if (component.empty() || component == "." || component == "..") throw std::invalid_argument("Invalid SI path component");
                start = i + 1;
            } else if (static_cast<unsigned char>(entry.Path[i]) < 0x20 || static_cast<unsigned char>(entry.Path[i]) > 0x7e || entry.Path[i] == '\\' || entry.Path[i] == ':') {
                throw std::invalid_argument("Unsupported SI path character");
            }
        }
        if (!paths.insert(entry.Path).second) throw std::invalid_argument("Duplicate SI entry path");
        total = Pfs::ImageWriter::Add(total, Pfs::ImageWriter::Add(76 + entry.Path.size() * 2, entry.Content.size()));
    }
    if (total >= 0xffffffff || total > std::numeric_limits<std::size_t>::max()) throw std::length_error("SI archive exceeds ZIP32 range");
    std::vector<std::byte> result(static_cast<std::size_t>(total));
    std::vector<std::array<std::uint32_t, 2>> records;
    std::size_t cursor = 0;
    const auto put = [&result](std::size_t offset, std::uint64_t value, std::size_t width) { Pfs::ImageWriter::Put(result, offset, value, width); };
    const auto writeName = [&result](std::size_t offset, const std::string& name) {
        for (std::size_t i = 0; i < name.size(); ++i) result[offset + i] = static_cast<std::byte>(name[i]);
    };
    for (const auto& entry : entries) {
        const auto crc = Crypto::Crc32::Zip(entry.Content);
        records.push_back({static_cast<std::uint32_t>(cursor), crc});
        put(cursor, 0x04034b50, 4);
        put(cursor + 4, 20, 2);
        put(cursor + 12, 0x21, 2);
        put(cursor + 14, crc, 4);
        put(cursor + 18, entry.Content.size(), 4);
        put(cursor + 22, entry.Content.size(), 4);
        put(cursor + 26, entry.Path.size(), 2);
        writeName(cursor + 30, entry.Path);
        cursor += 30 + entry.Path.size();
        for (const auto byte : entry.Content) result[cursor++] = byte;
    }
    const auto directoryStart = cursor;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        put(cursor, 0x02014b50, 4);
        put(cursor + 6, 20, 2);
        put(cursor + 14, 0x21, 2);
        put(cursor + 16, records[i][1], 4);
        put(cursor + 20, entry.Content.size(), 4);
        put(cursor + 24, entry.Content.size(), 4);
        put(cursor + 28, entry.Path.size(), 2);
        put(cursor + 42, records[i][0], 4);
        writeName(cursor + 46, entry.Path);
        cursor += 46 + entry.Path.size();
    }
    put(cursor, 0x06054b50, 4);
    put(cursor + 8, entries.size(), 2);
    put(cursor + 10, entries.size(), 2);
    put(cursor + 12, cursor - directoryStart, 4);
    put(cursor + 16, directoryStart, 4);
    return result;
}

}
