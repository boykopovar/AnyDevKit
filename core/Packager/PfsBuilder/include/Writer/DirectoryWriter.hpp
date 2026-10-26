#ifndef ANYDEVKIT_PFS_DIRECTORYWRITER_HPP
#define ANYDEVKIT_PFS_DIRECTORYWRITER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace AnyDevKit::Pfs {

class DirectoryWriter final {
public:
    static void Append(std::vector<std::byte>& data, std::uint32_t inode, std::uint32_t type, const std::string& name);
};

}

#endif
