#ifndef ANYDEVKIT_PS5_ELF_WRITER_HPP
#define ANYDEVKIT_PS5_ELF_WRITER_HPP

#include "Ps5ElfBuilder.hpp"

namespace AnyDevKit::Ps5 {

class ElfWriter final {
public:
    static std::vector<std::byte> Write(std::span<const std::byte> input, const ImportCatalog& imports, const Ps5ElfOptions& options);
};

}

#endif
