#include "Ps5ElfBuilder.hpp"
#include "Writer/ElfWriter.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>

namespace AnyDevKit {

std::vector<std::byte> Ps5ElfBuilder::Build(std::span<const std::byte> linkedElf, const ImportCatalog& imports, const Ps5ElfOptions& options) {
    auto validName = [](const std::string& value) { return !value.empty() && value.find_first_of("/\\\0", 0, 3) == std::string::npos; };
    if (!validName(options.ModuleName) || !validName(options.FileName) || options.SdkVersion == 0 || options.CompanionSdkVersion == 0) throw std::invalid_argument("PS5 module names and SDK versions must be specified");
    return Ps5::ElfWriter::Write(linkedElf, imports, options);
}

void Ps5ElfBuilder::BuildFile(const std::filesystem::path& input, const std::filesystem::path& output, const ImportCatalog& imports, const Ps5ElfOptions& options) {
    if (std::filesystem::exists(output) && std::filesystem::equivalent(input, output)) throw std::invalid_argument("PS5 output must differ from input ELF");
    const auto size = std::filesystem::file_size(input);
    if (size > std::numeric_limits<std::size_t>::max() || size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("ELF input is too large");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    std::ifstream source(input, std::ios::binary);
    source.exceptions(std::ios::badbit | std::ios::failbit);
    source.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    source.close();
    const auto result = Build(bytes, imports, options);
    if (result.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("PS5 ELF output is too large");
    std::ofstream destination(output, std::ios::binary | std::ios::trunc);
    destination.exceptions(std::ios::badbit | std::ios::failbit);
    destination.write(reinterpret_cast<const char*>(result.data()), static_cast<std::streamsize>(result.size()));
    destination.close();
}

}
