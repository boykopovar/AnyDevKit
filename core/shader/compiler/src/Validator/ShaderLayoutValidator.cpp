#include "Validator/ShaderLayoutValidator.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <glslang/Include/Types.h>

namespace AnyDevKit {

namespace {

void validateBinding(const glslang::TObjectReflection& resource, PsbcDescriptorType descriptorType, const PsbcCompileOptions& options) {
    const auto* type = resource.getType();
    if (!type) throw std::runtime_error("GLSL: resource reflection has no type");
    const auto& qualifier = type->getQualifier();
    if (!qualifier.hasSet() || resource.getBinding() < 0) throw std::invalid_argument("GLSL: resource requires an explicit set and binding: " + resource.name);
    for (std::uint32_t index = 0; index < options.descriptor_binding_count; ++index) {
        const auto& binding = options.descriptor_bindings[index];
        if (binding.set != qualifier.layoutSet || binding.binding != resource.getBinding()) continue;
        if (binding.type != descriptorType) throw std::invalid_argument("GLSL: descriptor type disagrees with the supplied layout: " + resource.name);
        return;
    }
    throw std::invalid_argument("GLSL: descriptor is missing from the supplied layout: " + resource.name);
}

PsbcDescriptorType descriptorType(const glslang::TSampler& sampler) {
    if (sampler.isSubpass()) return PSBC_DESCRIPTOR_INPUT_ATTACHMENT;
    if (sampler.isPureSampler()) return PSBC_DESCRIPTOR_SAMPLER;
    if (sampler.isImage()) return sampler.dim == glslang::EsdBuffer ? PSBC_DESCRIPTOR_STORAGE_TEXEL_BUFFER : PSBC_DESCRIPTOR_STORAGE_IMAGE;
    if (sampler.dim == glslang::EsdBuffer) return PSBC_DESCRIPTOR_UNIFORM_TEXEL_BUFFER;
    if (sampler.isCombined()) return PSBC_DESCRIPTOR_COMBINED_IMAGE_SAMPLER;
    if (sampler.isTexture()) return PSBC_DESCRIPTOR_SAMPLED_IMAGE;
    throw std::invalid_argument("GLSL: unsupported descriptor kind");
}

}

void ShaderLayoutValidator::Validate(glslang::TProgram& program, const PsbcCompileOptions& options) {
    if (!program.buildReflection(EShReflectionSeparateBuffers)) throw std::runtime_error("GLSL: resource reflection failed");
    for (int index = 0; index < program.getNumUniformBlocks(); ++index) {
        const auto& resource = program.getUniformBlock(index);
        if (!resource.getType()) throw std::runtime_error("GLSL: uniform block reflection has no type");
        if (resource.getType()->getQualifier().isPushConstant()) continue;
        validateBinding(resource, PSBC_DESCRIPTOR_UNIFORM_BUFFER, options);
    }
    for (int index = 0; index < program.getNumBufferBlocks(); ++index) validateBinding(program.getBufferBlock(index), PSBC_DESCRIPTOR_STORAGE_BUFFER, options);
    for (int index = 0; index < program.getNumUniformVariables(); ++index) {
        const auto& resource = program.getUniform(index);
        const auto* type = resource.getType();
        if (!type) throw std::runtime_error("GLSL: uniform reflection has no type");
        if (type->getBasicType() == glslang::EbtSampler) validateBinding(resource, descriptorType(type->getSampler()), options);
    }
}

}
