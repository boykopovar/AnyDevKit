#include "IO/Files.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
std::filesystem::path Files::Path(const std::string& value) {
    std::u8string text;
    for (unsigned char c : value) text.push_back(static_cast<char8_t>(c));
    return std::filesystem::path(text);
}
std::string Files::Text(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return {text.begin(), text.end()};
}
std::vector<std::byte> Files::Read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Cannot open " + Text(path));
    const auto size = stream.tellg();
    if (size < 0 || static_cast<std::uintmax_t>(size) > std::numeric_limits<std::size_t>::max() || static_cast<std::uintmax_t>(size) > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) throw std::runtime_error("Invalid file size: " + Text(path));
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    if (!bytes.empty()) stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!stream) throw std::runtime_error("Cannot read " + Text(path));
    if (stream.rdbuf()->sgetc() != std::char_traits<char>::eof()) throw std::runtime_error("File changed while reading: " + Text(path));
    return bytes;
}
void Files::RequireNew(const std::filesystem::path& path) {
    if (path.empty() || std::filesystem::exists(std::filesystem::symlink_status(path))) throw std::invalid_argument("Output already exists or is empty: " + Text(path));
    auto parent = path.parent_path();
    if (parent.empty()) parent = ".";
    if (!std::filesystem::is_directory(parent)) throw std::invalid_argument("Output parent does not exist: " + Text(parent));
}
void Files::Write(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    RequireNew(path);
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("Output exceeds stream size");
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot create " + Text(path));
    if (!bytes.empty()) stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    if (!stream) throw std::runtime_error("Cannot write " + Text(path));
}
void Files::RequireOutside(const std::filesystem::path& path, const std::filesystem::path& directory) {
    auto candidate = std::filesystem::weakly_canonical(path);
    const auto root = std::filesystem::canonical(directory);
    for (;;) {
        if (std::filesystem::exists(candidate) && std::filesystem::equivalent(candidate, root)) throw std::invalid_argument("Output must be outside input directory: " + Text(path));
        const auto parent = candidate.parent_path();
        if (parent == candidate || parent.empty()) break;
        candidate = parent;
    }
}
}
