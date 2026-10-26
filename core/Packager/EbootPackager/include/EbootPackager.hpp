#ifndef ANYDEVKIT_CORE_PACKAGER_EBOOTPACKAGER_INCLUDE_EBOOTPACKAGER_HPP
#define ANYDEVKIT_CORE_PACKAGER_EBOOTPACKAGER_INCLUDE_EBOOTPACKAGER_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace AnyDevKit {

struct EbootPackageOptions {
    std::uint64_t ProgramAuthorityId = 0x3100000000000002;
    std::uint64_t ApplicationVersion = 0;
    std::uint64_t FirmwareVersion = 0;
};

class EbootPackager final {
public:
    static std::vector<std::byte> Build(std::span<const std::byte> elf, const EbootPackageOptions& options = {});
    static void PackFile(const std::filesystem::path& input, const std::filesystem::path& output, const EbootPackageOptions& options = {});
};

}

#endif
