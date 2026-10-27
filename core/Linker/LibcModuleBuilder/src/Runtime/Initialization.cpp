#include "Runtime/Initialization.hpp"
#include "Writer/BinaryWriter.hpp"

#include <stdexcept>

namespace AnyDevKit::Libc {

std::vector<std::byte> Initialization::Build(std::uint64_t address, std::uint64_t mallocInitSlot) {
    const auto next = Ps5::BinaryWriter::Add(address, 10);
    if ((mallocInitSlot >= next && mallocInitSlot - next > 0x7fffffff) || (mallocInitSlot < next && next - mallocInitSlot > 0x80000000)) throw std::invalid_argument("Libc initialization import exceeds RIP-relative range");
    std::vector<std::byte> result;
    for (auto value : {0x48, 0x83, 0xec, 0x08, 0xff, 0x15}) result.push_back(static_cast<std::byte>(value));
    Ps5::BinaryWriter::Append(result, static_cast<std::uint32_t>(mallocInitSlot - next), 4);
    for (auto value : {0x85, 0xc0, 0x74, 0x02, 0x0f, 0x0b, 0x48, 0x83, 0xc4, 0x08, 0xc3}) result.push_back(static_cast<std::byte>(value));
    return result;
}

}
