#include "Shader/SpirvInput.hpp"
#include "IO/Files.hpp"
#include <stdexcept>

namespace AnyDevKit::PublicSdk {

std::vector<std::uint32_t> ReadSpirv(const std::filesystem::path& path) {
    const auto bytes = Files::Read(path);
    if (bytes.size() < 20 || bytes.size() % 4 != 0) throw std::invalid_argument("SPIR-V size must contain a complete header and words");
    std::vector<std::uint32_t> words(bytes.size() / 4);
    for (std::size_t i = 0; i < words.size(); ++i) for (unsigned j = 0; j < 4; ++j) words[i] |= std::to_integer<std::uint32_t>(bytes[4 * i + j]) << (8 * j);
    if (words[0] != 0x07230203u) throw std::invalid_argument("Invalid SPIR-V magic");
    return words;
}

}
