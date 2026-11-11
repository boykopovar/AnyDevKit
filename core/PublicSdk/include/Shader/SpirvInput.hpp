#ifndef ANYDEVKIT_PUBLICSDK_SPIRV_INPUT_HPP
#define ANYDEVKIT_PUBLICSDK_SPIRV_INPUT_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

namespace AnyDevKit::PublicSdk {

std::vector<std::uint32_t> ReadSpirv(const std::filesystem::path& path);

}

#endif
