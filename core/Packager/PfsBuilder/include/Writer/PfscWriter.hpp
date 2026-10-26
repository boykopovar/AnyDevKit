#ifndef ANYDEVKIT_PACKAGER_PFS_PFSCWRITER_HPP
#define ANYDEVKIT_PACKAGER_PFS_PFSCWRITER_HPP

#include "Writer/ImageWriter.hpp"

namespace AnyDevKit::Pfs {

class PfscWriter final {
public:
    static std::uint64_t StoredSize(std::uint64_t size);
    static void Write(ImageWriter& writer, std::uint64_t offset, const std::filesystem::path& source, std::uint64_t size);
};

}

#endif
