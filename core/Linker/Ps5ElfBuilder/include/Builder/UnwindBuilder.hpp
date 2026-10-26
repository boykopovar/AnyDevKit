#ifndef ANYDEVKIT_PS5_UNWIND_BUILDER_HPP
#define ANYDEVKIT_PS5_UNWIND_BUILDER_HPP

#include "Builder/StartupBuilder.hpp"

namespace AnyDevKit::Ps5 {

struct UnwindImage {
    std::vector<std::byte> Bytes;
    std::size_t HeaderOffset;
    std::size_t HeaderSize;
};

class UnwindBuilder final {
public:
    static UnwindImage Build(const ElfImage& image, const StartupImage& startup, std::uint64_t codeAddress, std::uint64_t address);
};

}

#endif
