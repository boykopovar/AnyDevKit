#ifndef ANYDEVKIT_PKG_NAPSWRITER_HPP
#define ANYDEVKIT_PKG_NAPSWRITER_HPP

#include "Builder/InnerImageBuilder.hpp"

#include <cstddef>
#include <vector>

namespace AnyDevKit::Pkg {

class NapsWriter final {
public:
    static std::vector<std::byte> Build(const InnerImageInfo& image);
};

}

#endif
