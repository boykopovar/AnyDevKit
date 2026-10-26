#include "EbootPackager.hpp"
#include "Reader/ElfReader.hpp"
#include "Writer/SelfWriter.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>

namespace AnyDevKit {

std::vector<std::byte> EbootPackager::Build(std::span<const std::byte> elf, const EbootPackageOptions& options) {
    return Self::SelfWriter::Write(Self::ElfReader::Read(elf), options);
}

void EbootPackager::PackFile(const std::filesystem::path& input, const std::filesystem::path& output, const EbootPackageOptions& options) {
    if (std::filesystem::exists(output) && std::filesystem::equivalent(input, output)) throw std::invalid_argument("SELF output must differ from ELF input");
    const auto size = std::filesystem::file_size(input);
    if (size > std::numeric_limits<std::size_t>::max() || size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("ELF input is too large");
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    std::ifstream source(input, std::ios::binary);
    source.exceptions(std::ios::badbit | std::ios::failbit);
    source.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    source.close();
    const auto package = Build(bytes, options);
    if (package.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("SELF output is too large");
    std::ofstream destination(output, std::ios::binary | std::ios::trunc);
    destination.exceptions(std::ios::badbit | std::ios::failbit);
    destination.write(reinterpret_cast<const char*>(package.data()), static_cast<std::streamsize>(package.size()));
    destination.close();
}

}
