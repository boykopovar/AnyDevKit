#ifndef ANYDEVKIT_PUBLICSDK_FILES_HPP
#define ANYDEVKIT_PUBLICSDK_FILES_HPP
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>
namespace AnyDevKit::PublicSdk {
class Files final {
public:
    static std::filesystem::path Path(const std::string& value);
    static std::string Text(const std::filesystem::path& path);
    static std::vector<std::byte> Read(const std::filesystem::path& path);
    static void Write(const std::filesystem::path& path, std::span<const std::byte> bytes);
    static void RequireNew(const std::filesystem::path& path);
    static void RequireOutside(const std::filesystem::path& path, const std::filesystem::path& directory);
};
}
#endif
