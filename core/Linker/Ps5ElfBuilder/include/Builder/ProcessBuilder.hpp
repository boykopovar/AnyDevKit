#ifndef ANYDEVKIT_PS5_PROCESS_BUILDER_HPP
#define ANYDEVKIT_PS5_PROCESS_BUILDER_HPP

#include "Ps5ElfBuilder.hpp"
#include "Reader/ElfImage.hpp"

namespace AnyDevKit::Ps5 {

struct ProcessImage {
    std::vector<std::byte> Bytes;
    std::vector<Relocation> Relocations;
};

class ProcessBuilder final {
public:
    static ProcessImage Build(std::uint64_t address, const Ps5ElfOptions& options);
};

}

#endif
