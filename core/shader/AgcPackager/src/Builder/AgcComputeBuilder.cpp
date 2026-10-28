#include "Builder/AgcComputeBuilder.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace AnyDevKit {

PsbcShaderMetadata AgcComputeBuilder::ReadMetadata(const PsbcShaderOutput& compiled) {
    if (compiled.metadata.source_stage != PSBC_STAGE_COMPUTE || !compiled.data || compiled.size < 0x58) throw std::invalid_argument("AGC: incomplete compute container");
    const auto* bytes = static_cast<const std::byte*>(compiled.data);
    const auto read = [&](std::size_t offset) {
        std::uint32_t value;
        std::memcpy(&value, bytes + offset, sizeof(value));
        return value;
    };
    const auto headerSize = ((read(0x2c) >> 8) & 255u) * 4u;
    if (headerSize < 0x24 || headerSize > compiled.size - 0x34) throw std::invalid_argument("AGC: invalid compute header size");
    if (read(0x24) != 0x72646853u || read(0x28) != 0x00020007u || (read(0x2c) & 0xffu) != 4u || read(0x3c) != headerSize || read(0x40) != 0) throw std::invalid_argument("AGC: incompatible compute container layout");
    if (compiled.machine_code_size > compiled.size - 0x34 - headerSize || !compiled.machine_code || std::memcmp(bytes + 0x34 + headerSize, compiled.machine_code, compiled.machine_code_size) != 0) throw std::invalid_argument("AGC: compute container and machine code disagree");
    auto metadata = compiled.metadata;
    if (metadata.shader_register_count || metadata.context_register_count || metadata.hardware_stage != PSBC_HW_STAGE_UNKNOWN) throw std::invalid_argument("AGC: unexpected compute metadata layout");
    const std::array<std::uint32_t, 3> threads{read(0x4c), read(0x50), read(0x54)};
    std::uint64_t total = 1;
    for (const auto dimension : threads) {
        if (!dimension || dimension > 1024) throw std::invalid_argument("AGC: invalid compute workgroup dimension");
        total *= dimension;
    }
    if (total > 1024) throw std::invalid_argument("AGC: compute workgroup exceeds 1024 threads");
    const std::array<PsbcRegisterWrite, 7> registers{{{0x20c, 0, 0}, {0x20d, 0, 0}, {0x212, 0, read(0x44)}, {0x213, 0, read(0x48)}, {0x207, 0, threads[0]}, {0x208, 0, threads[1]}, {0x209, 0, threads[2]}}};
    for (const auto entry : registers) metadata.shader_registers[metadata.shader_register_count++] = entry;
    return metadata;
}

}
