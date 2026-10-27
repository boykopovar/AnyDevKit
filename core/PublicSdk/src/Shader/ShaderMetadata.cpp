#include "Shader/ShaderMetadata.hpp"
#include "CommandLine/Arguments.hpp"
#include <limits>
#include <map>
#include <stdexcept>
#include <type_traits>
namespace AnyDevKit::PublicSdk {
namespace {
static_assert(PSBC_SHADER_METADATA_VERSION == 21);
template<class TValue, class TVisitor>
void Visit(const std::string& name, TValue& value, const TVisitor& visitor) {
    using Value = std::remove_cv_t<TValue>;
    if constexpr (std::is_array_v<Value>) {
        for (std::size_t i = 0; i < std::extent_v<Value>; ++i) Visit(name + "[" + std::to_string(i) + "]", value[i], visitor);
    } else if constexpr (std::is_same_v<Value, PsbcRegisterWrite>) {
        Visit(name + ".offset", value.offset, visitor);
        Visit(name + ".padding", value.padding, visitor);
        Visit(name + ".value", value.value, visitor);
    } else if constexpr (std::is_same_v<Value, PsbcDescriptorBinding>) {
        Visit(name + ".set", value.set, visitor);
        Visit(name + ".binding", value.binding, visitor);
        Visit(name + ".type", value.type, visitor);
        Visit(name + ".array_size", value.array_size, visitor);
        Visit(name + ".offset", value.offset, visitor);
        Visit(name + ".stride", value.stride, visitor);
    } else visitor(name, value);
}
template<class TValue, class TVisitor>
void Fields(TValue& metadata, const TVisitor& visitor) {
    Visit("version", metadata.version, visitor);
    Visit("target", metadata.target, visitor);
    Visit("source_stage", metadata.source_stage, visitor);
    Visit("hardware_stage", metadata.hardware_stage, visitor);
    Visit("unresolved_fields", metadata.unresolved_fields, visitor);
    Visit("context_register_count", metadata.context_register_count, visitor);
    Visit("context_registers", metadata.context_registers, visitor);
    Visit("shader_register_count", metadata.shader_register_count, visitor);
    Visit("shader_registers", metadata.shader_registers, visitor);
    Visit("linkage_valid", metadata.linkage_valid, visitor);
    Visit("linkage_ge_cntl", metadata.linkage_ge_cntl, visitor);
    Visit("linkage_stages_en", metadata.linkage_stages_en, visitor);
    Visit("linkage_user_vgpr_en", metadata.linkage_user_vgpr_en, visitor);
    Visit("merged_geometry", metadata.merged_geometry, visitor);
    Visit("merged_es_source_stage", metadata.merged_es_source_stage, visitor);
    Visit("merged_es_itemsize", metadata.merged_es_itemsize, visitor);
    Visit("merged_esgs_ring_itemsize", metadata.merged_esgs_ring_itemsize, visitor);
    Visit("linkage_ge_pc_alloc_valid", metadata.linkage_ge_pc_alloc_valid, visitor);
    Visit("linkage_ge_pc_alloc", metadata.linkage_ge_pc_alloc, visitor);
    Visit("hull_ls_valid", metadata.hull_ls_valid, visitor);
    Visit("hull_ls_code_size", metadata.hull_ls_code_size, visitor);
    Visit("hull_ls_code_offset", metadata.hull_ls_code_offset, visitor);
    Visit("hull_ls_pgm_lo", metadata.hull_ls_pgm_lo, visitor);
    Visit("hull_ls_pgm_hi", metadata.hull_ls_pgm_hi, visitor);
    Visit("hull_ls_rsrc1", metadata.hull_ls_rsrc1, visitor);
    Visit("hull_ls_rsrc2", metadata.hull_ls_rsrc2, visitor);
    Visit("hull_tess_wg_valid", metadata.hull_tess_wg_valid, visitor);
    Visit("hull_num_patches_per_wg", metadata.hull_num_patches_per_wg, visitor);
    Visit("hull_tcs_lds_size", metadata.hull_tcs_lds_size, visitor);
    Visit("hull_workgroup_size", metadata.hull_workgroup_size, visitor);
    Visit("ps_clip_distance_reads", metadata.ps_clip_distance_reads, visitor);
    Visit("ps_cull_distance_reads", metadata.ps_cull_distance_reads, visitor);
    Visit("esgs_system_sgprs_valid", metadata.esgs_system_sgprs_valid, visitor);
    Visit("esgs_gs_tg_info_sgpr", metadata.esgs_gs_tg_info_sgpr, visitor);
    Visit("esgs_merged_wave_info_sgpr", metadata.esgs_merged_wave_info_sgpr, visitor);
    Visit("esgs_es_verts_per_subgroup", metadata.esgs_es_verts_per_subgroup, visitor);
    Visit("esgs_gs_inst_prims_per_subgroup", metadata.esgs_gs_inst_prims_per_subgroup, visitor);
    Visit("esgs_prim_amp_factor", metadata.esgs_prim_amp_factor, visitor);
    Visit("esgs_workgroup_size", metadata.esgs_workgroup_size, visitor);
    Visit("user_data_window_base", metadata.user_data_window_base, visitor);
    Visit("input_semantic_count", metadata.input_semantic_count, visitor);
    Visit("input_semantics", metadata.input_semantics, visitor);
    Visit("output_semantic_count", metadata.output_semantic_count, visitor);
    Visit("output_semantics", metadata.output_semantics, visitor);
    Visit("clip_distance_mask", metadata.clip_distance_mask, visitor);
    Visit("cull_distance_mask", metadata.cull_distance_mask, visitor);
    Visit("address32_hi", metadata.address32_hi, visitor);
    Visit("user_sgpr_count", metadata.user_sgpr_count, visitor);
    Visit("vertex_buffer_table_valid", metadata.vertex_buffer_table_valid, visitor);
    Visit("vertex_buffer_table_user_data_dword", metadata.vertex_buffer_table_user_data_dword, visitor);
    Visit("descriptor_set0_valid", metadata.descriptor_set0_valid, visitor);
    Visit("descriptor_set0_user_data_dword", metadata.descriptor_set0_user_data_dword, visitor);
    Visit("descriptor_set_valid", metadata.descriptor_set_valid, visitor);
    Visit("descriptor_set_user_data_dword", metadata.descriptor_set_user_data_dword, visitor);
    Visit("push_constants_valid", metadata.push_constants_valid, visitor);
    Visit("push_constants_user_data_dword", metadata.push_constants_user_data_dword, visitor);
    Visit("push_constant_size", metadata.push_constant_size, visitor);
    Visit("push_use_valid", metadata.push_use_valid, visitor);
    Visit("stage_push_dwords", metadata.stage_push_dwords, visitor);
    Visit("previous_stage_push_dwords", metadata.previous_stage_push_dwords, visitor);
    Visit("hull_push_use_valid", metadata.hull_push_use_valid, visitor);
    Visit("hull_vertex_push_dwords", metadata.hull_vertex_push_dwords, visitor);
    Visit("hull_control_push_dwords", metadata.hull_control_push_dwords, visitor);
    Visit("descriptor_binding_count", metadata.descriptor_binding_count, visitor);
    Visit("descriptor_bindings", metadata.descriptor_bindings, visitor);
    Visit("descriptor_used_binding_mask", metadata.descriptor_used_binding_mask, visitor);
    Visit("base_vertex_valid", metadata.base_vertex_valid, visitor);
    Visit("base_vertex_user_data_dword", metadata.base_vertex_user_data_dword, visitor);
    Visit("draw_id_valid", metadata.draw_id_valid, visitor);
    Visit("draw_id_user_data_dword", metadata.draw_id_user_data_dword, visitor);
    Visit("view_index_valid", metadata.view_index_valid, visitor);
    Visit("view_index_user_data_dword", metadata.view_index_user_data_dword, visitor);
    Visit("start_instance_valid", metadata.start_instance_valid, visitor);
    Visit("start_instance_user_data_dword", metadata.start_instance_user_data_dword, visitor);
    Visit("streamout_valid", metadata.streamout_valid, visitor);
    Visit("streamout_buffer_table_user_data_dword", metadata.streamout_buffer_table_user_data_dword, visitor);
    Visit("streamout_enabled_stream_buffers_mask", metadata.streamout_enabled_stream_buffers_mask, visitor);
    Visit("streamout_strides_dwords", metadata.streamout_strides_dwords, visitor);
    Visit("streamout_config_sgpr", metadata.streamout_config_sgpr, visitor);
    Visit("streamout_write_index_sgpr", metadata.streamout_write_index_sgpr, visitor);
    Visit("streamout_offset_sgprs", metadata.streamout_offset_sgprs, visitor);
    Visit("scratch_valid", metadata.scratch_valid, visitor);
    Visit("scratch_bytes_per_wave", metadata.scratch_bytes_per_wave, visitor);
    Visit("scratch_size_per_thread", metadata.scratch_size_per_thread, visitor);
    Visit("scratch_buffer_table_user_data_dword", metadata.scratch_buffer_table_user_data_dword, visitor);
    Visit("ps5_ring_table_valid", metadata.ps5_ring_table_valid, visitor);
    Visit("ps5_ring_table_user_data_dword", metadata.ps5_ring_table_user_data_dword, visitor);
    Visit("ngg_lds_layout_valid", metadata.ngg_lds_layout_valid, visitor);
    Visit("ngg_lds_layout_user_data_dword", metadata.ngg_lds_layout_user_data_dword, visitor);
    Visit("ngg_lds_layout", metadata.ngg_lds_layout, visitor);
    Visit("vertex_buffer_usage_mask", metadata.vertex_buffer_usage_mask, visitor);
    Visit("vertex_buffer_per_attribute", metadata.vertex_buffer_per_attribute, visitor);
}
}
std::string ShaderMetadata::Encode(const PsbcShaderMetadata& metadata) {
    std::string output = "adk-psbc-metadata-v1\n";
    Fields(metadata, [&](const std::string& name, const auto& value) {
        output += name + "=" + std::to_string(static_cast<std::uint64_t>(value)) + "\n";
    });
    return output;
}
PsbcShaderMetadata ShaderMetadata::Decode(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 == text.size() || text[i + 1] != '\n') throw std::invalid_argument("Invalid shader metadata line ending");
        } else normalized.push_back(text[i]);
    }
    text = normalized;
    constexpr std::string_view magic = "adk-psbc-metadata-v1\n";
    if (!text.starts_with(magic)) throw std::invalid_argument("Unsupported shader metadata format");
    text.remove_prefix(magic.size());
    std::map<std::string, std::uint64_t> values;
    while (!text.empty()) {
        const auto end = text.find('\n');
        if (end == std::string_view::npos) throw std::invalid_argument("Truncated shader metadata");
        const auto line = text.substr(0, end);
        const auto split = line.find('=');
        if (split == std::string_view::npos || !values.emplace(line.substr(0, split), Arguments::Number(std::string(line.substr(split + 1)))).second) throw std::invalid_argument("Invalid or duplicate shader metadata field");
        text.remove_prefix(end + 1);
    }
    PsbcShaderMetadata metadata{};
    Fields(metadata, [&](const std::string& name, auto& value) {
        auto entry = values.extract(name);
        if (entry.empty()) throw std::invalid_argument("Missing shader metadata field: " + name);
        using Value = std::remove_reference_t<decltype(value)>;
        std::uint64_t maximum;
        if constexpr (std::is_enum_v<Value>) maximum = static_cast<std::uint64_t>(std::numeric_limits<std::underlying_type_t<Value>>::max());
        else maximum = static_cast<std::uint64_t>(std::numeric_limits<Value>::max());
        if (entry.mapped() > maximum) throw std::invalid_argument("Shader metadata value exceeds field range: " + name);
        value = static_cast<Value>(entry.mapped());
    });
    if (!values.empty()) throw std::invalid_argument("Unknown shader metadata field: " + values.begin()->first);
    return metadata;
}
}
