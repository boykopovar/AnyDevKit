#ifndef ANYDEVKIT_PKG_FIHWRITER_HPP
#define ANYDEVKIT_PKG_FIHWRITER_HPP

#include "Builder/InnerImageBuilder.hpp"
#include "Builder/OuterImageBuilder.hpp"

namespace AnyDevKit::Pkg {

class FihWriter final {
public:
    static std::vector<std::byte> Build(const InnerImageInfo& inner, const OuterImageInfo& outer, std::span<const std::byte> naps, std::uint32_t contentVersion);
};

}

#endif
