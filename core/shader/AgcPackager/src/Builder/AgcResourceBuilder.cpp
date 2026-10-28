#include "Builder/AgcResourceBuilder.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <SceShaders.hpp>

namespace AnyDevKit {

namespace {

std::uint32_t descriptorStride(PsbcDescriptorType type) {
    switch (type) {
        case PSBC_DESCRIPTOR_UNIFORM_BUFFER:
        case PSBC_DESCRIPTOR_UNIFORM_TEXEL_BUFFER:
        case PSBC_DESCRIPTOR_STORAGE_BUFFER:
        case PSBC_DESCRIPTOR_STORAGE_TEXEL_BUFFER:
        case PSBC_DESCRIPTOR_SAMPLER:
            return 16;
        case PSBC_DESCRIPTOR_INPUT_ATTACHMENT:
        case PSBC_DESCRIPTOR_STORAGE_IMAGE:
        case PSBC_DESCRIPTOR_SAMPLED_IMAGE:
            return 32;
        case PSBC_DESCRIPTOR_COMBINED_IMAGE_SAMPLER:
            return 48;
        default:
            throw std::invalid_argument("AGC: unsupported descriptor type");
    }
}

}

void AgcResourceBuilder::Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer) {
    if (metadata.user_sgpr_count > 16 || metadata.descriptor_binding_count > PSBC_MAX_DESCRIPTOR_BINDINGS) throw std::invalid_argument("AGC: invalid resource table size");
    std::array<bool, 16> occupied{};
    const auto claim = [&](bool valid, std::uint32_t slot) {
        if (!valid) {
            if (slot) throw std::invalid_argument("AGC: slot supplied for an undeclared resource");
            return;
        }
        if (slot >= metadata.user_sgpr_count || occupied[slot]) throw std::invalid_argument("AGC: user-data slot is outside the stage window or overlaps another resource");
        occupied[slot] = true;
    };
    claim(metadata.vertex_buffer_table_valid, metadata.vertex_buffer_table_user_data_dword);
    claim(metadata.push_constants_valid, metadata.push_constants_user_data_dword);
    claim(metadata.base_vertex_valid, metadata.base_vertex_user_data_dword);
    claim(metadata.start_instance_valid, metadata.start_instance_user_data_dword);
    claim(metadata.draw_id_valid, metadata.draw_id_user_data_dword);
    claim(metadata.view_index_valid, metadata.view_index_user_data_dword);
    claim(metadata.ngg_lds_layout_valid, metadata.ngg_lds_layout_user_data_dword);
    if (metadata.vertex_buffer_table_valid != (metadata.vertex_buffer_usage_mask != 0)) throw std::invalid_argument("AGC: inconsistent vertex buffer usage mask");
    if (!metadata.vertex_buffer_table_valid && metadata.vertex_buffer_per_attribute) throw std::invalid_argument("AGC: vertex attribute layout without a vertex buffer table");
    if ((metadata.start_instance_valid || metadata.draw_id_valid) && !metadata.base_vertex_valid) throw std::invalid_argument("AGC: incomplete draw-parameter block");
    if (metadata.push_constants_valid) {
        if (!metadata.push_constant_size || metadata.push_constant_size > 256) throw std::invalid_argument("AGC: unsupported push constant size");
    } else if (metadata.push_constant_size) {
        throw std::invalid_argument("AGC: push constant size without a pointer");
    }
    if (!metadata.push_use_valid && (metadata.stage_push_dwords || metadata.previous_stage_push_dwords)) throw std::invalid_argument("AGC: push constant usage without a valid declaration");
    if (metadata.previous_stage_push_dwords && !metadata.merged_geometry) throw std::invalid_argument("AGC: unexpected previous-stage push constant usage");
    const auto pushDwords = (metadata.push_constant_size + 3u) / 4u;
    const auto pushMask = pushDwords == 64 ? std::numeric_limits<std::uint64_t>::max() : (std::uint64_t{1} << pushDwords) - 1;
    if ((metadata.stage_push_dwords | metadata.previous_stage_push_dwords) & ~pushMask) throw std::invalid_argument("AGC: push constant usage exceeds its declared size");
    if (metadata.descriptor_set0_valid != metadata.descriptor_set_valid[0] || metadata.descriptor_set0_user_data_dword != metadata.descriptor_set_user_data_dword[0]) throw std::invalid_argument("AGC: descriptor set zero aliases disagree");
    std::array<std::uint64_t, PSBC_MAX_DESCRIPTOR_SETS> bindings{};
    for (std::uint32_t index = 0; index < metadata.descriptor_binding_count; ++index) {
        const auto& binding = metadata.descriptor_bindings[index];
        const auto stride = descriptorStride(binding.type);
        if (binding.set >= PSBC_MAX_DESCRIPTOR_SETS || binding.binding >= 64 || !binding.array_size || binding.stride != stride || binding.offset % 16 || binding.array_size > (std::numeric_limits<std::uint32_t>::max() - binding.offset) / stride) throw std::invalid_argument("AGC: invalid descriptor layout");
        const auto mask = std::uint64_t{1} << binding.binding;
        if (bindings[binding.set] & mask) throw std::invalid_argument("AGC: duplicate descriptor binding");
        bindings[binding.set] |= mask;
        const auto end = binding.offset + binding.array_size * stride;
        for (std::uint32_t previous = 0; previous < index; ++previous) {
            const auto& other = metadata.descriptor_bindings[previous];
            if (other.set == binding.set && binding.offset < other.offset + other.array_size * other.stride && other.offset < end) throw std::invalid_argument("AGC: descriptor arrays overlap");
        }
    }
    for (std::size_t set = 0; set < PSBC_MAX_DESCRIPTOR_SETS; ++set) {
        claim(metadata.descriptor_set_valid[set], metadata.descriptor_set_user_data_dword[set]);
        if (metadata.descriptor_set_valid[set] && !bindings[set]) throw std::invalid_argument("AGC: active descriptor set has no bindings");
        if (metadata.descriptor_used_binding_mask[set] & ~bindings[set]) throw std::invalid_argument("AGC: descriptor usage names an undeclared binding");
    }
    const auto resources = writer.Reserve(sizeof(ShaderUserData), alignof(ShaderUserData));
    writer.WriteRelativePointer(offsetof(Shader, user_data), resources);
}

}
