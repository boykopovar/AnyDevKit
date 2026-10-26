#ifndef ANYDEVKIT_PKG_INNERIMAGEBUILDER_HPP
#define ANYDEVKIT_PKG_INNERIMAGEBUILDER_HPP

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace AnyDevKit::Pkg {

struct InnerFileInfo {
    std::string Path;
    std::uint64_t Offset = 0;
    std::uint64_t Size = 0;
    std::uint32_t Inode = 0;
    bool Executable = false;
};

struct InnerImageInfo {
    std::uint64_t Size = 0;
    std::uint64_t MetadataOffset = 0;
    std::uint32_t InodeCount = 0;
    std::uint32_t ApplicationFiles = 0;
    std::vector<InnerFileInfo> Files;
};

class InnerImageBuilder final {
public:
    static InnerImageInfo Build(const std::filesystem::path& input, const std::filesystem::path& output, std::int64_t timestamp, std::span<const std::string> excludedPaths = {});
};

}

#endif
