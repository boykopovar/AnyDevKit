#ifndef ANYDEVKIT_PS5_BINARY_WRITER_HPP
#define ANYDEVKIT_PS5_BINARY_WRITER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace AnyDevKit::Ps5 {

class BinaryWriter final {
public:
    static std::vector<std::byte> HashTable(std::span<const std::string> names);
    static std::uint64_t Add(std::uint64_t left, std::uint64_t right);
    static std::uint64_t Align(std::uint64_t value, std::uint64_t alignment);
    static void Set(std::span<std::byte> output, std::uint64_t offset, std::uint64_t value, std::size_t size);
    static void Append(std::vector<std::byte>& output, std::uint64_t value, std::size_t size);
    static std::uint32_t String(std::vector<std::byte>& output, std::string_view value);
private:
    static std::uint32_t hash(std::string_view name);
};

}

#endif
