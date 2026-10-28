#include "AgcPackager.hpp"

#include "Builder/AgcRegisterBuilder.hpp"
#include "Builder/AgcResourceBuilder.hpp"
#include "Builder/AgcSemanticBuilder.hpp"
#include "Validator/AgcMetadataValidator.hpp"
#include "Writer/AgcBinaryWriter.hpp"
#include <cstdint>
#include <cstring>
#include <utility>
#include <SceShaders.hpp>
#include <prx/libSceAgc/Shader/include/ShaderConstants.hpp>

namespace AnyDevKit {

static_assert(alignof(Shader) == AgcShaderPackage::HeaderAlignment);
static_assert((ShaderRegs::SHADER_BASE_ALIGN_MASK & 0xffffu) + 1 == AgcShaderPackage::CodeAlignment);

AgcShaderPackage::AgcShaderPackage(std::vector<std::byte> headerBytes, std::vector<std::byte> codeBytes, const PsbcShaderMetadata& metadata) : headerBytes(std::move(headerBytes)), codeBytes(std::move(codeBytes)), metadata(metadata) {}

std::span<const std::byte> AgcShaderPackage::GetHeaderBytes() const { return headerBytes; }
std::span<const std::byte> AgcShaderPackage::GetCodeBytes() const { return codeBytes; }
const PsbcShaderMetadata& AgcShaderPackage::GetBindingMetadata() const { return metadata; }

AgcShaderPackage AgcPackager::Build(const PsbcShaderOutput& compiled) {
    AgcMetadataValidator::Validate(compiled);
    AgcBinaryWriter writer;
    writer.Reserve(sizeof(Shader), alignof(Shader));
    writer.WriteUInt32(offsetof(Shader, file_header), ShaderRegs::SHADER_FILE_HEADER_MAGIC);
    writer.WriteUInt32(offsetof(Shader, version), ShaderRegs::SHADER_VERSION);
    writer.WriteUInt32(offsetof(Shader, shader_size), static_cast<std::uint32_t>(compiled.machine_code_size));
    writer.WriteUInt32(offsetof(Shader, target), 5);
    const auto type = compiled.metadata.hardware_stage == PSBC_HW_STAGE_NGG ? ShaderRegs::ShaderBinaryType::Gs : ShaderRegs::ShaderBinaryType::Ps;
    writer.WriteUInt8(offsetof(Shader, type), static_cast<std::uint8_t>(type));
    AgcRegisterBuilder::Build(compiled.metadata, writer);
    AgcResourceBuilder::Build(compiled.metadata, writer);
    AgcSemanticBuilder::Build(compiled.metadata, writer);
    writer.Reserve(0, AgcShaderPackage::HeaderAlignment);
    writer.WriteUInt32(offsetof(Shader, header_size), static_cast<std::uint32_t>(writer.GetSize()));
    std::vector<std::byte> machineCode(compiled.machine_code_size);
    std::memcpy(machineCode.data(), compiled.machine_code, machineCode.size());
    return AgcShaderPackage(std::move(writer).Finish(), std::move(machineCode), compiled.metadata);
}

}
