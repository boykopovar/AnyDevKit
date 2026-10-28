#include "Builder/AgcRegisterBuilder.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <SceShaders.hpp>
#include <prx/libSceAgc/Shader/include/ShaderConstants.hpp>

namespace AnyDevKit {

namespace {

void validateRegisters(std::span<const PsbcRegisterWrite> registers) {
    for (std::size_t index = 0; index < registers.size(); ++index) {
        if (registers[index].padding) throw std::invalid_argument("AGC: nonzero register padding");
        for (std::size_t previous = 0; previous < index; ++previous) {
            if (registers[previous].offset == registers[index].offset) throw std::invalid_argument("AGC: duplicate register offset");
        }
    }
}

const PsbcRegisterWrite& requireRegister(std::span<const PsbcRegisterWrite> registers, std::uint32_t offset) {
    for (const auto& entry : registers) {
        if (entry.offset == offset) return entry;
    }
    throw std::invalid_argument("AGC: required stage register is missing");
}

void writeRegister(AgcBinaryWriter& writer, std::size_t offset, const PsbcRegisterWrite& entry) {
    writer.WriteUInt32(offset + offsetof(ShaderRegister, offset), entry.offset);
    writer.WriteUInt32(offset + offsetof(ShaderRegister, value), entry.value);
}

}

void AgcRegisterBuilder::Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer) {
    const bool hull = metadata.hardware_stage == PSBC_HW_STAGE_HULL;
    const bool compute = metadata.source_stage == PSBC_STAGE_COMPUTE;
    if ((!compute && !metadata.context_register_count) || metadata.context_register_count > PSBC_MAX_CONTEXT_REGISTERS || !metadata.shader_register_count || metadata.shader_register_count > PSBC_MAX_SHADER_REGISTERS) throw std::invalid_argument("AGC: invalid register count");
    const std::span<const PsbcRegisterWrite> context(metadata.context_registers, metadata.context_register_count);
    const std::span<const PsbcRegisterWrite> shader(metadata.shader_registers, metadata.shader_register_count);
    validateRegisters(context);
    validateRegisters(shader);
    const bool vertex = metadata.hardware_stage == PSBC_HW_STAGE_NGG;
    const auto programOffset = hull ? ShaderRegs::SPI_SHADER_PGM_LO_LS : compute ? ShaderRegs::COMPUTE_PGM_LO : vertex ? ShaderRegs::SPI_SHADER_PGM_LO_ES : ShaderRegs::SPI_SHADER_PGM_LO_PS;
    const auto& low = requireRegister(shader, programOffset);
    const auto& high = requireRegister(shader, programOffset + 1);
    if (&low + 1 != &high || low.value || high.value) throw std::invalid_argument("AGC: program registers must be adjacent and unrelocated");
    const std::uint32_t resourceOffset = hull ? 0x10au : compute ? 0x212u : vertex ? 0x8au : 0xau;
    requireRegister(shader, resourceOffset);
    const auto& resources = requireRegister(shader, resourceOffset + 1);
    if (resources.value & 1u) throw std::invalid_argument("AGC: scratch-enabled shader registers are unsupported");
    if (((resources.value >> 1) & 31u) != metadata.user_sgpr_count) throw std::invalid_argument("AGC: register and metadata user SGPR counts disagree");
    if (vertex) {
        for (const auto offset : std::array<std::uint32_t, 10>{0x1b1u, 0x1c2u, 0x1c3u, 0x207u, 0x2d3u, 0x2e4u, 0x291u, 0x1ffu, 0x2abu, 0x2ceu}) requireRegister(context, offset);
        requireRegister(shader, 0x87u);
        requireRegister(shader, ShaderRegs::SPI_SHADER_PGM_RSRC4_GS);
        if (!metadata.linkage_valid || metadata.linkage_ge_cntl.offset != ShaderRegs::GE_CNTL || metadata.linkage_stages_en.offset != ShaderRegs::VGT_SHADER_STAGES_EN || metadata.linkage_user_vgpr_en.offset != ShaderRegs::GE_USER_VGPR_EN || metadata.linkage_ge_cntl.padding || metadata.linkage_stages_en.padding || metadata.linkage_user_vgpr_en.padding) throw std::invalid_argument("AGC: invalid vertex linkage registers");
        if (!(metadata.unresolved_fields & PSBC_UNRESOLVED_NGG_ESGS_RING_ITEMSIZE) && requireRegister(context, 0x2abu).value != (metadata.merged_geometry ? metadata.merged_esgs_ring_itemsize : 1)) throw std::invalid_argument("AGC: inconsistent resolved NGG ring item size");
        if (metadata.esgs_system_sgprs_valid) {
            const auto onchip = requireRegister(context, ShaderRegs::VGT_GS_ONCHIP_CNTL).value;
            if ((onchip & 0x7ffu) != metadata.esgs_es_verts_per_subgroup || ((onchip >> 22) & 0x3ffu) != metadata.esgs_gs_inst_prims_per_subgroup || (requireRegister(context, ShaderRegs::GE_NGG_SUBGRP_CNTL).value & 0x1ffu) != metadata.esgs_prim_amp_factor) throw std::invalid_argument("AGC: NGG launch registers disagree with system metadata");
        }
    } else if (hull) {
        requireRegister(context, 0x2dbu);
    } else if (!compute) {
        for (const auto offset : std::array<std::uint32_t, 9>{0x1c4u, 0x1c5u, 0x1b3u, 0x1b4u, 0x1b6u, 0x1b8u, 0x203u, 0x8fu, 0x310u}) requireRegister(context, offset);
        for (const auto& entry : {metadata.linkage_ge_cntl, metadata.linkage_stages_en, metadata.linkage_user_vgpr_en}) {
            if (entry.offset || entry.padding || entry.value) throw std::invalid_argument("AGC: unexpected fragment linkage register");
        }
    }
    const auto contextOffset = writer.Reserve(context.size() * sizeof(ShaderRegister), alignof(ShaderRegister));
    const auto shaderOffset = writer.Reserve(shader.size() * sizeof(ShaderRegister), alignof(ShaderRegister));
    for (std::size_t index = 0; index < context.size(); ++index) {
        auto entry = context[index];
        if (vertex && entry.offset == 0x2abu) entry.value = metadata.merged_geometry ? metadata.merged_esgs_ring_itemsize : 1;
        writeRegister(writer, contextOffset + index * sizeof(ShaderRegister), entry);
    }
    for (std::size_t index = 0; index < shader.size(); ++index) writeRegister(writer, shaderOffset + index * sizeof(ShaderRegister), shader[index]);
    if (!context.empty()) writer.WriteRelativePointer(offsetof(Shader, cx_registers), contextOffset);
    writer.WriteRelativePointer(offsetof(Shader, sh_registers), shaderOffset);
    writer.WriteUInt8(offsetof(Shader, num_cx_registers), static_cast<std::uint8_t>(context.size()));
    writer.WriteUInt8(offsetof(Shader, num_sh_registers), static_cast<std::uint8_t>(shader.size()));
    if (vertex || compute || hull) {
        const auto specials = writer.Reserve(sizeof(ShaderSpecialRegs), alignof(ShaderSpecialRegs));
        if (compute) writer.WriteUInt32(specials + offsetof(ShaderSpecialRegs, dispatch_modifier), 0x8000u);
        writeRegister(writer, specials + offsetof(ShaderSpecialRegs, ge_cntl), metadata.linkage_ge_cntl);
        writeRegister(writer, specials + offsetof(ShaderSpecialRegs, vgt_shader_stages_en), metadata.linkage_stages_en);
        writeRegister(writer, specials + offsetof(ShaderSpecialRegs, ge_user_vgpr_en), metadata.linkage_user_vgpr_en);
        writer.WriteRelativePointer(offsetof(Shader, specials), specials);
        writer.WriteUInt16(offsetof(Shader, special_sizes_bytes), sizeof(ShaderSpecialRegs));
    }
}

}
