#ifndef ANYDEVKIT_PACKAGER_PFS_IMAGEWRITER_HPP
#define ANYDEVKIT_PACKAGER_PFS_IMAGEWRITER_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

namespace AnyDevKit::Pfs {

class ImageWriter final {
public:
    explicit ImageWriter(const std::filesystem::path& output);
    void Write(std::uint64_t offset, std::span<const std::byte> bytes);
    void Copy(std::uint64_t offset, const std::filesystem::path& input, std::uint64_t size);
    void Zero(std::uint64_t offset, std::uint64_t size);
    void Close();
    static std::uint64_t Add(std::uint64_t left, std::uint64_t right);
    static std::uint64_t Align(std::uint64_t value, std::uint64_t alignment);
    static void Put(std::span<std::byte> bytes, std::size_t offset, std::uint64_t value, std::size_t width);
private:
    std::ofstream stream;
};

}

#endif
