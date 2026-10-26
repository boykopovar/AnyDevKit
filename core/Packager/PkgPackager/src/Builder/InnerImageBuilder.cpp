#include "Builder/InnerImageBuilder.hpp"
#include "PfsBuilder.hpp"
#include "Writer/DirectoryWriter.hpp"
#include "Writer/FlatPathWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>

namespace AnyDevKit::Pkg {
namespace {

using Pfs::ImageWriter;
constexpr std::uint64_t blockSize = 0x10000;
constexpr std::uint64_t inodeSize = 0xa8;

struct Node {
    PfsEntry Entry;
    std::uint64_t Offset = 0;
    std::uint64_t Size = 0;
    std::uint32_t Parent = 0xffffffff;
    std::uint32_t Dirent = 0xffffffff;
    std::uint32_t Afid = 0xffffffff;
    std::uint32_t Flags = 0x20010;
    std::uint16_t Mode = 0x816d;
    std::uint16_t Links = 1;
    std::vector<std::byte> Data;
};

std::string ParentPath(const std::string& path) {
    const auto slash = path.rfind('/');
    return slash == std::string::npos ? "" : path.substr(0, slash);
}

std::string Leaf(const std::string& path) {
    const auto slash = path.rfind('/');
    return path.substr(slash == std::string::npos ? 0 : slash + 1);
}

bool SystemPath(const std::string& path) {
    return path.starts_with("sce_sys/");
}

}

InnerImageInfo InnerImageBuilder::Build(const std::filesystem::path& input, const std::filesystem::path& output, std::int64_t timestamp, std::span<const std::string> excludedPaths) {
    if (timestamp < 0) throw std::invalid_argument("Negative inner PFS timestamp");
    if (std::filesystem::exists(std::filesystem::symlink_status(output))) throw std::invalid_argument("Inner PFS output already exists");
    auto entries = PfsBuilder::ScanDirectory(input);
    std::erase_if(entries, [&](const auto& entry) { return std::find(excludedPaths.begin(), excludedPaths.end(), entry.Path) != excludedPaths.end(); });
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.Directory > b.Directory; });
    if (entries.size() > 0xfffffa) throw std::length_error("Too many inner PFS inodes");
    std::vector<Node> nodes(5);
    nodes[0].Mode = 0x416d;
    nodes[4].Mode = 0x416d;
    nodes[4].Flags = 0x10;
    nodes[4].Links = 3;
    nodes[0].Entry.Directory = true;
    nodes[4].Entry.Directory = true;
    std::map<std::string, std::uint32_t> directories{{"", 4}};
    std::set<std::string> names;
    for (const auto& entry : entries) {
        auto normalized = entry.Path;
        for (auto& c : normalized) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        if (!names.insert(normalized).second) throw std::invalid_argument("Case-insensitive PFS path collision: " + entry.Path);
        const auto parent = directories.find(ParentPath(entry.Path));
        if (parent == directories.end()) throw std::invalid_argument("Missing inner PFS parent: " + entry.Path);
        Node node;
        node.Entry = entry;
        node.Parent = parent->second;
        if (entry.Directory) {
            node.Mode = 0x4168;
            node.Links = 2;
            if (nodes[parent->second].Links == 0xffff) throw std::length_error("Too many PFS directory links");
            ++nodes[parent->second].Links;
            directories.emplace(entry.Path, static_cast<std::uint32_t>(nodes.size()));
        } else {
            node.Size = std::filesystem::file_size(entry.Source);
            if (node.Size >= (std::uint64_t{1} << 40)) throw std::length_error("PFS file exceeds flat table size field");
            node.Mode = SystemPath(entry.Path) ? 0x8168 : 0x816d;
            std::array<unsigned char, 4> magic{};
            std::ifstream source(entry.Source, std::ios::binary);
            if (!source) throw std::runtime_error("Cannot read inner PFS source");
            if (node.Size >= 4) {
                source.read(reinterpret_cast<char*>(magic.data()), 4);
                if (!source) throw std::runtime_error("Cannot read inner PFS source magic");
            }
            const auto value = std::uint32_t{magic[0]} | (std::uint32_t{magic[1]} << 8) | (std::uint32_t{magic[2]} << 16) | (std::uint32_t{magic[3]} << 24);
            const bool executable = value == 0x1d3d154f || value == 0xeef51454 || value == 0x464c457f;
            node.Flags = 0x10 | (executable ? 0x40 : 0x20) | (SystemPath(entry.Path) ? 0x20000 : 0);
        }
        nodes.push_back(std::move(node));
    }
    for (const auto& [path, index] : directories) {
        static_cast<void>(path);
        Pfs::DirectoryWriter::Append(nodes[index].Data, index, 4, ".");
        Pfs::DirectoryWriter::Append(nodes[index].Data, index == 4 ? 4 : nodes[index].Parent, 5, "..");
    }
    for (std::uint32_t i = 5; i < nodes.size(); ++i) {
        auto& node = nodes[i];
        auto& data = nodes[node.Parent].Data;
        const auto entrySize = ImageWriter::Align(17 + Leaf(node.Entry.Path).size(), 8);
        if (data.size() % blockSize + entrySize > blockSize) data.resize(static_cast<std::size_t>(ImageWriter::Align(data.size(), blockSize)));
        if (data.size() > 0xffffffff) throw std::length_error("PFS directory exceeds dirent offset field");
        node.Dirent = static_cast<std::uint32_t>(data.size());
        Pfs::DirectoryWriter::Append(data, i, node.Entry.Directory ? 3 : 2, Leaf(node.Entry.Path));
    }
    const std::array<std::string, 4> internalNames{"inode_flat_path_table", "apr_flat_path_table", "afid_to_ino_table", "uroot"};
    for (std::uint32_t i = 0; i < internalNames.size(); ++i) Pfs::DirectoryWriter::Append(nodes[0].Data, i + 1, i == 3 ? 3 : 2, internalNames[i]);
    std::vector<std::uint32_t> files;
    for (std::uint32_t i = 5; i < nodes.size(); ++i) if (!nodes[i].Entry.Directory) files.push_back(i);
    if (files.empty()) throw std::invalid_argument("Inner PFS contains no files");
    std::stable_sort(files.begin(), files.end(), [&](auto a, auto b) { return SystemPath(nodes[a].Entry.Path) > SystemPath(nodes[b].Entry.Path); });
    InnerImageInfo result;
    std::uint64_t cursor = 0;
    nodes[3].Data.resize((files.size() + 3) * 4);
    ImageWriter::Put(nodes[3].Data, 0, *std::min_element(files.begin(), files.end()), 4);
    for (std::size_t i = 0; i < files.size(); ++i) {
        auto& node = nodes[files[i]];
        node.Afid = static_cast<std::uint32_t>(i);
        node.Offset = cursor;
        cursor = ImageWriter::Align(ImageWriter::Add(cursor, std::max<std::uint64_t>(1, node.Size)), 0x40000);
        result.Files.push_back({node.Entry.Path, node.Offset, node.Size, files[i]});
        if (!SystemPath(node.Entry.Path)) ++result.ApplicationFiles;
        ImageWriter::Put(nodes[3].Data, 4 + i * 4, files[i], 4);
    }
    ImageWriter::Put(nodes[3].Data, nodes[3].Data.size() - 8, 0xffffffffffffffff, 8);
    std::vector<std::pair<std::uint64_t, std::uint64_t>> inodePaths, applicationPaths;
    for (std::size_t i = 5; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const bool directory = node.Entry.Directory;
        const auto hash = Pfs::FlatPathWriter::Hash(node.Entry.Path);
        const auto afid = directory ? 0xffffff : node.Afid;
        const auto flags = (directory ? 0x40000000ULL : 0) | (directory || SystemPath(node.Entry.Path) ? 0x80000000ULL : 0);
        inodePaths.emplace_back(hash, i | flags | (std::uint64_t{afid} << 40));
        if (!directory && !SystemPath(node.Entry.Path)) applicationPaths.emplace_back(hash, node.Size | (std::uint64_t{afid} << 40));
    }
    nodes[1].Data = Pfs::FlatPathWriter::Build(inodePaths);
    nodes[2].Data = Pfs::FlatPathWriter::Build(applicationPaths);
    result.MetadataOffset = ImageWriter::Align(ImageWriter::Add(cursor, 61 * blockSize), 0x40000);
    constexpr auto inodesPerBlock = blockSize / inodeSize;
    const auto inodeBlocks = (nodes.size() + inodesPerBlock - 1) / inodesPerBlock;
    cursor = ImageWriter::Add(result.MetadataOffset, (1 + inodeBlocks) * blockSize);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        auto& node = nodes[i];
        if (i >= 5 && !node.Entry.Directory) continue;
        node.Offset = cursor;
        node.Size = node.Entry.Directory ? ImageWriter::Align(node.Data.size(), blockSize) : node.Data.size();
        cursor = ImageWriter::Add(cursor, ImageWriter::Align(node.Data.size(), blockSize));
    }
    result.Size = ImageWriter::Add(cursor, blockSize);
    result.InodeCount = static_cast<std::uint32_t>(nodes.size());
    if (result.Size >= (std::uint64_t{1} << 40)) throw std::length_error("Inner PFS exceeds NAPS offset range");
    try {
        ImageWriter writer(output);
        writer.Zero(0, result.Size);
        for (const auto index : files) writer.Copy(nodes[index].Offset, nodes[index].Entry.Source, nodes[index].Size);
        for (const auto& node : nodes) if (!node.Data.empty()) writer.Write(node.Offset, node.Data);
        std::array<std::byte, blockSize> superblock{};
        ImageWriter::Put(superblock, 0, 2, 8);
        ImageWriter::Put(superblock, 8, 20130315, 8);
        ImageWriter::Put(superblock, 0x1a, 1, 1);
        ImageWriter::Put(superblock, 0x1c, 0x18, 2);
        ImageWriter::Put(superblock, 0x20, blockSize, 4);
        ImageWriter::Put(superblock, 0x28, 1, 8);
        ImageWriter::Put(superblock, 0x30, nodes.size(), 8);
        ImageWriter::Put(superblock, 0x38, result.Size / blockSize, 8);
        ImageWriter::Put(superblock, 0x40, inodeBlocks, 8);
        for (const auto offset : {0x50, 0x58, 0x60}) ImageWriter::Put(superblock, offset, blockSize, 4);
        ImageWriter::Put(superblock, 0x54, 0x10, 4);
        for (std::size_t i = 0; i < 4; ++i) ImageWriter::Put(superblock, 0x68 + i * 8, static_cast<std::uint64_t>(timestamp), 8);
        ImageWriter::Put(superblock, 0xb0, inodeBlocks, 8);
        ImageWriter::Put(superblock, 0xd8, result.MetadataOffset / blockSize + 1, 8);
        ImageWriter::Put(superblock, 0x368, 1, 1);
        writer.Write(result.MetadataOffset, superblock);
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            std::array<std::byte, inodeSize> inode{};
            ImageWriter::Put(inode, 0, node.Mode, 2);
            ImageWriter::Put(inode, 2, node.Links, 2);
            ImageWriter::Put(inode, 4, node.Flags, 4);
            ImageWriter::Put(inode, 8, node.Size, 8);
            ImageWriter::Put(inode, 0x10, node.Size, 8);
            for (std::size_t t = 0; t < 4; ++t) ImageWriter::Put(inode, 0x18 + t * 8, static_cast<std::uint64_t>(timestamp), 8);
            ImageWriter::Put(inode, 0x60, node.Offset, 8);
            ImageWriter::Put(inode, 0x68, node.Afid, 4);
            ImageWriter::Put(inode, 0x6c, node.Parent, 4);
            ImageWriter::Put(inode, 0x70, node.Dirent, 4);
            writer.Write(result.MetadataOffset + blockSize + i / inodesPerBlock * blockSize + i % inodesPerBlock * inodeSize, inode);
        }
        writer.Close();
    } catch (...) {
        if (std::filesystem::exists(output)) std::filesystem::remove(output);
        throw;
    }
    return result;
}

}
