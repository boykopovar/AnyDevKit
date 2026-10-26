#include "Writer/DirectoryWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <limits>
#include <stdexcept>

namespace AnyDevKit::Pfs {

void DirectoryWriter::Append(std::vector<std::byte>& data, std::uint32_t inode, std::uint32_t type, const std::string& name) {
    if (name.empty() || name.find('\0') != std::string::npos || type < 2 || type > 5) throw std::invalid_argument("Invalid PFS directory entry");
    const auto size = ImageWriter::Align(ImageWriter::Add(name.size(), 17), 8);
    const auto start = data.size();
    if (size > std::numeric_limits<std::uint32_t>::max() || size > data.max_size() - start) throw std::length_error("PFS directory is too large");
    data.resize(start + static_cast<std::size_t>(size));
    ImageWriter::Put(data, start, inode, 4);
    ImageWriter::Put(data, start + 4, type, 4);
    ImageWriter::Put(data, start + 8, name.size(), 4);
    ImageWriter::Put(data, start + 12, size, 4);
    for (std::size_t i = 0; i < name.size(); ++i) data[start + 16 + i] = static_cast<std::byte>(name[i]);
}

}
