#ifndef ANYDEVKIT_APPMETA_DDSWRITER_HPP
#define ANYDEVKIT_APPMETA_DDSWRITER_HPP

#include "IconBuilder.hpp"

namespace AnyDevKit::Icon {

class DdsWriter final {
public:
    static std::vector<std::byte> Build(const RgbaImage& image);
};

}

#endif
