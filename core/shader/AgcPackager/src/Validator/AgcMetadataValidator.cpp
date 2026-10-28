#include "Validator/AgcMetadataValidator.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <SceShaders.hpp>
#include <prx/libSceAgc/Shader/include/ShaderConstants.hpp>

namespace AnyDevKit {

static_assert(PSBC_SHADER_METADATA_VERSION == 21);
static_assert(sizeof(void*) == 8);
static_assert(std::endian::native == std::endian::little);
static_assert(std::is_standard_layout_v<Shader> && sizeof(Shader) == 96);
static_assert(std::is_standard_layout_v<ShaderUserData> && sizeof(ShaderUserData) == 56);
static_assert(std::is_standard_layout_v<ShaderSpecialRegs> && sizeof(ShaderSpecialRegs) == 48);
static_assert(sizeof(ShaderSemantic) == 4 && sizeof(ShaderRegister) == 8);
static_assert(offsetof(Shader, header_size) == 64 && offsetof(Shader, type) == 90);

void AgcMetadataValidator::Validate(const PsbcShaderOutput& compiled) {
    const auto& metadata = compiled.metadata;
    if (!compiled.machine_code || compiled.machine_code_size == 0 || compiled.machine_code_size % 4 != 0 || compiled.machine_code_size > std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("AGC: invalid machine code buffer or size");
    if (metadata.version != PSBC_SHADER_METADATA_VERSION || metadata.target != PSBC_TARGET_PS5) throw std::invalid_argument("AGC: incompatible PSBC metadata version or target");
    const bool vertex = (metadata.source_stage == PSBC_STAGE_VERTEX || metadata.source_stage == PSBC_STAGE_GEOMETRY) && metadata.hardware_stage == PSBC_HW_STAGE_NGG;
    const bool fragment = metadata.source_stage == PSBC_STAGE_FRAGMENT && metadata.hardware_stage == PSBC_HW_STAGE_PIXEL;
    const bool compute = metadata.source_stage == PSBC_STAGE_COMPUTE && metadata.hardware_stage == PSBC_HW_STAGE_UNKNOWN;
    if (!vertex && !fragment && !compute) throw std::invalid_argument("AGC: invalid hardware stage for the source stage");
    const std::uint32_t permitted = PSBC_UNRESOLVED_PROGRAM_CHECKSUM | (vertex ? static_cast<std::uint32_t>(PSBC_UNRESOLVED_NGG_ESGS_RING_ITEMSIZE) : 0u);
    if (metadata.unresolved_fields & ~permitted) throw std::invalid_argument("AGC: unresolved compiler metadata");
    if (metadata.scratch_valid || metadata.scratch_bytes_per_wave || metadata.scratch_size_per_thread || metadata.scratch_buffer_table_user_data_dword) throw std::invalid_argument("AGC: scratch storage is unsupported");
    if (metadata.source_stage == PSBC_STAGE_GEOMETRY) {
        if (!metadata.merged_geometry || metadata.merged_es_source_stage != PSBC_STAGE_VERTEX || !metadata.merged_es_itemsize || !metadata.merged_esgs_ring_itemsize || !metadata.esgs_system_sgprs_valid) throw std::invalid_argument("AGC: incomplete merged geometry metadata");
    } else if (metadata.merged_geometry || metadata.merged_es_source_stage != PSBC_STAGE_NONE || metadata.merged_es_itemsize || metadata.merged_esgs_ring_itemsize) throw std::invalid_argument("AGC: unexpected merged geometry metadata");
    if (metadata.hull_ls_valid || metadata.hull_ls_code_size || metadata.hull_ls_code_offset || metadata.hull_tess_wg_valid || metadata.hull_num_patches_per_wg || metadata.hull_tcs_lds_size || metadata.hull_workgroup_size || metadata.hull_push_use_valid || metadata.hull_vertex_push_dwords || metadata.hull_control_push_dwords || metadata.ps5_ring_table_valid || metadata.ps5_ring_table_user_data_dword) throw std::invalid_argument("AGC: tessellation metadata is unsupported");
    for (const auto& entry : {metadata.hull_ls_pgm_lo, metadata.hull_ls_pgm_hi, metadata.hull_ls_rsrc1, metadata.hull_ls_rsrc2}) {
        if (entry.offset || entry.padding || entry.value) throw std::invalid_argument("AGC: unexpected hull registers");
    }
    if (metadata.streamout_valid || metadata.streamout_buffer_table_user_data_dword || metadata.streamout_enabled_stream_buffers_mask || metadata.streamout_config_sgpr || metadata.streamout_write_index_sgpr) throw std::invalid_argument("AGC: streamout is unsupported");
    for (std::size_t index = 0; index < 4; ++index) {
        if (metadata.streamout_strides_dwords[index] || metadata.streamout_offset_sgprs[index]) throw std::invalid_argument("AGC: unexpected streamout layout");
    }
    if (metadata.clip_distance_mask || metadata.cull_distance_mask || metadata.ps_clip_distance_reads || metadata.ps_cull_distance_reads) throw std::invalid_argument("AGC: clip and cull distance linkage is unsupported");
    if (metadata.address32_hi != 2 || metadata.user_sgpr_count > 16 || metadata.user_data_window_base > 16 || metadata.user_sgpr_count > 16 - metadata.user_data_window_base) throw std::invalid_argument("AGC: unsupported PS5 user-data address or SGPR window");
    if (metadata.linkage_valid != vertex || metadata.ngg_lds_layout_valid != vertex) throw std::invalid_argument("AGC: stage linkage or NGG LDS layout is missing or unexpected");
    if (metadata.ngg_lds_layout > std::numeric_limits<std::uint16_t>::max()) throw std::invalid_argument("AGC: NGG LDS layout exceeds the supported range");
    if (!vertex && (metadata.ngg_lds_layout || metadata.user_data_window_base || metadata.base_vertex_valid || metadata.start_instance_valid || metadata.draw_id_valid || metadata.vertex_buffer_table_valid)) throw std::invalid_argument("AGC: vertex-only metadata on a fragment shader");
    if (metadata.esgs_system_sgprs_valid) {
        if (!vertex || !metadata.user_data_window_base || metadata.esgs_gs_tg_info_sgpr >= metadata.user_data_window_base || metadata.esgs_merged_wave_info_sgpr >= metadata.user_data_window_base || metadata.esgs_gs_tg_info_sgpr == metadata.esgs_merged_wave_info_sgpr || !metadata.esgs_es_verts_per_subgroup || !metadata.esgs_gs_inst_prims_per_subgroup || !metadata.esgs_prim_amp_factor || !metadata.esgs_workgroup_size || metadata.esgs_workgroup_size > 256) throw std::invalid_argument("AGC: invalid NGG system SGPR layout");
    } else if (metadata.esgs_gs_tg_info_sgpr || metadata.esgs_merged_wave_info_sgpr || metadata.esgs_es_verts_per_subgroup || metadata.esgs_gs_inst_prims_per_subgroup || metadata.esgs_prim_amp_factor || metadata.esgs_workgroup_size) {
        throw std::invalid_argument("AGC: undeclared NGG system SGPR layout");
    }
    if (metadata.linkage_ge_pc_alloc_valid) {
        if (!vertex || metadata.linkage_ge_pc_alloc.offset != ShaderRegs::GE_PC_ALLOC || metadata.linkage_ge_pc_alloc.padding) throw std::invalid_argument("AGC: invalid parameter-cache allocation register");
    } else if (metadata.linkage_ge_pc_alloc.offset || metadata.linkage_ge_pc_alloc.padding || metadata.linkage_ge_pc_alloc.value) {
        throw std::invalid_argument("AGC: undeclared parameter-cache allocation register");
    }
    if ((!compute && metadata.context_register_count == 0) || metadata.context_register_count > PSBC_MAX_CONTEXT_REGISTERS || metadata.shader_register_count == 0 || metadata.shader_register_count > PSBC_MAX_SHADER_REGISTERS || metadata.input_semantic_count > PSBC_MAX_SEMANTICS || metadata.output_semantic_count > PSBC_MAX_SEMANTICS || metadata.descriptor_binding_count > PSBC_MAX_DESCRIPTOR_BINDINGS) throw std::invalid_argument("AGC: invalid metadata table count");
}

}
