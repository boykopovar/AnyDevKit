#include "PfsBuilder.hpp"
#include "Writer/ImageWriter.hpp"
#include "Writer/PfscWriter.hpp"
#include "Writer/DirectoryWriter.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace AnyDevKit {
namespace {

using Pfs::ImageWriter;

struct Node {
    PfsEntry Entry;
    std::uint64_t SourceSize = 0;
    std::uint64_t Size = 0;
    std::uint64_t Start = 0;
    std::uint32_t Blocks = 0;
    std::uint16_t Links = 1;
    bool Internal = false;
    std::vector<std::byte> Data;
};

std::string Normalize(const std::string& path, bool caseInsensitive) {
    std::string result = path;
    for (auto& c : result) if (caseInsensitive && c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return result;
}

void ValidatePath(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.back() == '/') throw std::invalid_argument("Invalid PFS path: " + path);
    std::size_t start = 0;
    for (std::size_t i = 0; i <= path.size(); ++i) {
        if (i == path.size() || path[i] == '/') {
            const auto component = path.substr(start, i - start);
            if (component.empty() || component == "." || component == ".." || component.size() > 255) throw std::invalid_argument("Invalid PFS path component: " + path);
            start = i + 1;
        } else if (static_cast<unsigned char>(path[i]) < 0x20 || static_cast<unsigned char>(path[i]) > 0x7e || path[i] == '\\' || path[i] == ':') {
            throw std::invalid_argument("Unsupported PFS path character: " + path);
        }
    }
}

std::string Parent(const std::string& path) {
    const auto separator = path.rfind('/');
    return separator == std::string::npos ? "" : path.substr(0, separator);
}

std::string Name(const std::string& path) {
    const auto separator = path.rfind('/');
    return separator == std::string::npos ? path : path.substr(separator + 1);
}

void AppendEntry(std::vector<std::byte>& data, std::uint32_t inode, std::uint32_t type, const std::string& name) {
    Pfs::DirectoryWriter::Append(data, inode, type, name);
}

std::uint32_t Hash(const std::string& path, bool caseInsensitive) {
    std::uint32_t hash = 0;
    for (const auto c : Normalize(path, caseInsensitive)) hash = hash * 31 + static_cast<unsigned char>(c);
    return hash;
}

}

std::vector<PfsEntry> PfsBuilder::ScanDirectory(const std::filesystem::path& input) {
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(input)) || !std::filesystem::is_directory(input)) throw std::invalid_argument("PFS input must be a real directory");
    std::vector<PfsEntry> entries;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(input)) {
        const auto status = entry.symlink_status();
        if (!std::filesystem::is_directory(status) && !std::filesystem::is_regular_file(status)) throw std::invalid_argument("PFS input contains a symbolic link or special file");
        const auto path = entry.path().lexically_relative(input).generic_string();
        ValidatePath(path);
        entries.push_back({path, entry.path(), entry.is_directory(), false});
    }
    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) { return left.Path < right.Path; });
    return entries;
}

void PfsBuilder::Build(const std::vector<PfsEntry>& entries, const std::filesystem::path& output, const PfsOptions& options) {
    const auto blockSize = options.BlockSize;
    if (blockSize < 0x1000 || blockSize > 0x100000 || (blockSize & (blockSize - 1))) throw std::invalid_argument("PFS block size must be a power of two between 4 KiB and 1 MiB");
    if (options.Timestamp < 0) throw std::invalid_argument("PFS timestamp must not be negative");
    if (std::filesystem::exists(std::filesystem::symlink_status(output))) throw std::invalid_argument("PFS output already exists");
    std::vector<PfsEntry> sorted = entries;
    std::sort(sorted.begin(), sorted.end(), [](const auto& left, const auto& right) {
        if (left.Directory != right.Directory) return left.Directory;
        return left.Path < right.Path;
    });
    std::set<std::string> paths;
    std::map<std::uint32_t, std::vector<std::size_t>> hashes;
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const auto& entry = sorted[i];
        ValidatePath(entry.Path);
        if (!paths.insert(Normalize(entry.Path, options.CaseInsensitive)).second) throw std::invalid_argument("Duplicate PFS path: " + entry.Path);
        if (entry.Directory && entry.Pfsc) throw std::invalid_argument("Directory cannot contain PFSC data");
        hashes[Hash("/" + entry.Path, options.CaseInsensitive)].push_back(i);
    }
    const bool collisions = std::any_of(hashes.begin(), hashes.end(), [](const auto& item) { return item.second.size() > 1; });
    const std::uint32_t root = collisions ? 3 : 2;
    if (sorted.size() >= 0x20000000 - root - 1) throw std::length_error("Too many PFS inodes");
    std::vector<Node> nodes(root + 1);
    nodes[0].Entry.Directory = true;
    nodes[0].Internal = true;
    nodes[1].Internal = true;
    if (collisions) nodes[2].Internal = true;
    nodes[root].Entry.Directory = true;
    nodes[root].Links = 3;
    std::map<std::string, std::uint32_t> directories{{"", root}};
    for (const auto& entry : sorted) {
        Node node;
        node.Entry = entry;
        if (entry.Directory) {
            directories.emplace(entry.Path, static_cast<std::uint32_t>(nodes.size()));
            node.Links = 2;
        } else {
            if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(entry.Source))) throw std::invalid_argument("PFS source is not a regular file");
            node.SourceSize = std::filesystem::file_size(entry.Source);
            node.Size = entry.Pfsc ? Pfs::PfscWriter::StoredSize(node.SourceSize) : node.SourceSize;
        }
        nodes.push_back(std::move(node));
    }
    for (const auto& [path, inode] : directories) {
        const auto parent = directories.find(Parent(path));
        if (parent == directories.end()) throw std::invalid_argument("PFS parent directory is missing: " + path);
        AppendEntry(nodes[inode].Data, inode, 4, ".");
        AppendEntry(nodes[inode].Data, parent->second, 5, "..");
    }
    for (std::size_t i = root + 1; i < nodes.size(); ++i) {
        const auto& entry = nodes[i].Entry;
        const auto parent = directories.find(Parent(entry.Path));
        if (parent == directories.end()) throw std::invalid_argument("PFS parent directory is missing: " + entry.Path);
        auto& directory = nodes[parent->second];
        AppendEntry(directory.Data, static_cast<std::uint32_t>(i), entry.Directory ? 3 : 2, Name(entry.Path));
        if (entry.Directory) {
            if (directory.Links == std::numeric_limits<std::uint16_t>::max()) throw std::length_error("Too many PFS directory links");
            ++directory.Links;
        }
    }
    AppendEntry(nodes[0].Data, 1, 2, "flat_path_table");
    if (collisions) AppendEntry(nodes[0].Data, 2, 2, "collision_resolver");
    AppendEntry(nodes[0].Data, root, 3, "uroot");
    for (const auto& [hash, indices] : hashes) {
        std::uint32_t value = 0;
        if (indices.size() == 1) {
            value = static_cast<std::uint32_t>(root + 1 + indices.front());
            if (sorted[indices.front()].Directory) value |= 0x20000000;
        } else {
            if (nodes[2].Data.size() >= 0x80000000) throw std::length_error("PFS collision table is too large");
            value = 0x80000000 | static_cast<std::uint32_t>(nodes[2].Data.size());
            for (const auto index : indices) AppendEntry(nodes[2].Data, static_cast<std::uint32_t>(root + 1 + index), sorted[index].Directory ? 3 : 2, "/" + sorted[index].Path);
            nodes[2].Data.resize(nodes[2].Data.size() + 0x18);
        }
        const auto offset = nodes[1].Data.size();
        nodes[1].Data.resize(offset + 8);
        ImageWriter::Put(nodes[1].Data, offset, hash, 4);
        ImageWriter::Put(nodes[1].Data, offset + 4, value, 4);
    }
    const std::uint64_t inodesPerBlock = blockSize / 0xa8;
    const auto inodeBlocks = (nodes.size() + inodesPerBlock - 1) / inodesPerBlock;
    std::uint64_t next = 1 + inodeBlocks;
    for (auto& node : nodes) {
        if (node.Entry.Directory || node.Internal) node.Size = node.Data.size();
        const auto blocks = std::max<std::uint64_t>(1, ImageWriter::Align(node.Size, blockSize) / blockSize);
        node.Start = next;
        next = ImageWriter::Add(next, blocks);
        if (next > 0x7fffffff) throw std::length_error("PFS image exceeds D32 block range");
        node.Blocks = static_cast<std::uint32_t>(blocks);
        if (node.Entry.Directory) node.Size = blocks * blockSize;
    }
    std::vector<std::byte> header(blockSize);
    ImageWriter::Put(header, 0, 2, 8);
    ImageWriter::Put(header, 8, 20130315, 8);
    ImageWriter::Put(header, 0x1a, 1, 1);
    ImageWriter::Put(header, 0x1c, options.CaseInsensitive ? 8 : 0, 2);
    ImageWriter::Put(header, 0x20, blockSize, 4);
    ImageWriter::Put(header, 0x28, 1, 8);
    ImageWriter::Put(header, 0x30, nodes.size(), 8);
    ImageWriter::Put(header, 0x38, next, 8);
    ImageWriter::Put(header, 0x40, inodeBlocks, 8);
    ImageWriter::Put(header, 0x52, 1, 2);
    ImageWriter::Put(header, 0x54, 0x10, 4);
    ImageWriter::Put(header, 0x58, inodeBlocks * blockSize, 8);
    ImageWriter::Put(header, 0x60, inodeBlocks * blockSize, 8);
    for (std::size_t i = 0; i < 4; ++i) ImageWriter::Put(header, 0x68 + i * 8, static_cast<std::uint64_t>(options.Timestamp), 8);
    ImageWriter::Put(header, 0xb0, inodeBlocks, 4);
    ImageWriter::Put(header, 0xd8, 1, 8);
    ImageWriter::Put(header, 0x368, 1, 4);
    try {
        ImageWriter writer(output);
        writer.Write(0, header);
        writer.Zero(blockSize, inodeBlocks * blockSize);
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const auto& node = nodes[i];
            std::vector<std::byte> inode(0xa8);
            ImageWriter::Put(inode, 0, (node.Entry.Directory ? 0x4000 : 0x8000) | 0555, 2);
            ImageWriter::Put(inode, 2, node.Links, 2);
            ImageWriter::Put(inode, 4, 0x10 | (node.Internal ? 0x20000 : 0) | (node.Entry.Pfsc ? 1 : 0), 4);
            ImageWriter::Put(inode, 8, node.Size, 8);
            ImageWriter::Put(inode, 16, node.Entry.Pfsc ? node.SourceSize : node.Size, 8);
            for (std::size_t time = 0; time < 4; ++time) ImageWriter::Put(inode, 0x18 + time * 8, static_cast<std::uint64_t>(options.Timestamp), 8);
            ImageWriter::Put(inode, 0x60, node.Blocks, 4);
            ImageWriter::Put(inode, 0x64, node.Start, 4);
            for (std::size_t pointer = 1; pointer < 12; ++pointer) ImageWriter::Put(inode, 0x64 + pointer * 4, 0xffffffff, 4);
            writer.Write((1 + i / inodesPerBlock) * blockSize + (i % inodesPerBlock) * 0xa8, inode);
            const auto offset = node.Start * blockSize;
            if (node.Entry.Directory || node.Internal) {
                writer.Write(offset, node.Data);
                writer.Zero(offset + node.Data.size(), static_cast<std::uint64_t>(node.Blocks) * blockSize - node.Data.size());
            } else {
                if (node.Entry.Pfsc) Pfs::PfscWriter::Write(writer, offset, node.Entry.Source, node.SourceSize);
                else writer.Copy(offset, node.Entry.Source, node.SourceSize);
                writer.Zero(offset + node.Size, static_cast<std::uint64_t>(node.Blocks) * blockSize - node.Size);
            }
        }
        writer.Close();
    } catch (...) {
        std::error_code error;
        std::filesystem::remove(output, error);
        throw;
    }
}

}
