#ifndef ANYDEVKIT_PKG_PLAYGOBUILDER_HPP
#define ANYDEVKIT_PKG_PLAYGOBUILDER_HPP

#include "Builder/InnerImageBuilder.hpp"
#include "Builder/OuterImageBuilder.hpp"

#include <string_view>

namespace AnyDevKit::Pkg {

struct PlayGoData {
    std::vector<std::byte> Chunks;
    std::vector<std::byte> Hashes;
    std::vector<std::byte> FileChunks;
};

class PlayGoBuilder final {
public:
    static PlayGoData Build(std::string_view contentId, const InnerImageInfo& inner, const OuterImageInfo& outer);
};

}

#endif
