#ifndef ANYDEVKIT_PS5_STARTUP_BUILDER_HPP
#define ANYDEVKIT_PS5_STARTUP_BUILDER_HPP

#include "Reader/ElfImage.hpp"

namespace AnyDevKit::Ps5 {

struct StartupImage {
    std::vector<std::byte> Bytes;
    std::uint64_t Entry;
};

class StartupBuilder final {
public:
    static StartupImage Build(const ElfImage& image, std::uint64_t codeAddress, std::uint64_t importSlots);
};

}

#endif
