#ifndef ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_WRITER_AGCBINARYWRITER_HPP
#define ANYDEVKIT_CORE_SHADER_AGCPACKAGER_INCLUDE_WRITER_AGCBINARYWRITER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyDevKit {

class AgcBinaryWriter {
public:
    std::size_t Reserve(std::size_t size, std::size_t alignment);
    void WriteUInt8(std::size_t offset, std::uint8_t value);
    void WriteUInt16(std::size_t offset, std::uint16_t value);
    void WriteUInt32(std::size_t offset, std::uint32_t value);
    void WriteUInt64(std::size_t offset, std::uint64_t value);
    void WriteBytes(std::size_t offset, std::span<const std::byte> source);
    void WriteRelativePointer(std::size_t fieldOffset, std::size_t targetOffset);
    std::size_t GetSize() const;
    std::vector<std::byte> Finish() &&;

private:
    void writeInteger(std::size_t offset, std::uint64_t value, std::size_t size);
    void requireRange(std::size_t offset, std::size_t size) const;
    std::vector<std::byte> bytes;
};

}

#endif
