#ifndef ANYDEVKIT_PKG_OUTERIMAGEBUILDER_HPP
#define ANYDEVKIT_PKG_OUTERIMAGEBUILDER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace AnyDevKit::Pkg {

struct OuterImageInfo {
    std::uint64_t Size = 0;
    std::uint32_t Superblock = 0;
    std::array<std::byte, 32> SuperblockDigest{};
    std::vector<std::byte> BlockDigests;
};

class OuterImageBuilder final {
public:
    static OuterImageInfo Build(const std::filesystem::path& inner, const std::filesystem::path& naps, std::uint64_t innerLogicalSize, std::span<const std::byte, 32> key, std::span<const std::byte, 16> seed, std::int64_t timestamp, const std::filesystem::path& output);
};

}

#endif
