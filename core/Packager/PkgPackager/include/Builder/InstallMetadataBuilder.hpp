#ifndef ANYDEVKIT_PKG_INSTALLMETADATABUILDER_HPP
#define ANYDEVKIT_PKG_INSTALLMETADATABUILDER_HPP

#include "Builder/InnerImageBuilder.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace AnyDevKit::Pkg {

struct InstallMetadata {
    std::vector<std::byte> Metrics;
    std::vector<std::byte> Descriptor;
};

class InstallMetadataBuilder final {
public:
    static InstallMetadata Build(const std::filesystem::path& imagePath, const InnerImageInfo& inner, std::span<const std::byte, 32> imageKey, std::span<const std::byte, 16> seed);
};

}

#endif
