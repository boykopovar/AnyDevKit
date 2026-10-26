#ifndef ANYDEVKIT_PS5_ELF_BUILDER_HPP
#define ANYDEVKIT_PS5_ELF_BUILDER_HPP

#include "Imports/ImportCatalog.hpp"

#include <filesystem>
#include <vector>

namespace AnyDevKit {

struct Ps5ElfOptions {
    std::string ModuleName;
    std::string FileName;
    std::uint32_t SdkVersion = 0;
    std::uint32_t CompanionSdkVersion = 0;
};

class Ps5ElfBuilder final {
public:
    static std::vector<std::byte> Build(std::span<const std::byte> linkedElf, const ImportCatalog& imports, const Ps5ElfOptions& options);
    static void BuildFile(const std::filesystem::path& input, const std::filesystem::path& output, const ImportCatalog& imports, const Ps5ElfOptions& options);
};

}

#endif
