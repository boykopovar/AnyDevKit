#include "Builder/AgcSemanticBuilder.hpp"

#include <cstddef>
#include <span>
#include <stdexcept>
#include <SceShaders.hpp>

namespace AnyDevKit {

namespace {

void writeSemantics(AgcBinaryWriter& writer, std::span<const std::uint32_t> words, std::size_t pointerOffset, bool output) {
    if (words.empty()) return;
    for (std::size_t index = 0; index < words.size(); ++index) {
        const auto word = words[index];
        const auto semantic = word & 0xffu;
        const auto mask = output ? 0x1fffu : 0x4000ffu;
        if ((word & ~mask) || !((semantic >= 15 && semantic <= PSBC_SEMANTIC_PRIMITIVE_ID) || semantic == PSBC_SEMANTIC_VIEWPORT_INDEX)) throw std::invalid_argument("AGC: unsupported semantic encoding");
        if (output) {
            for (std::size_t previous = 0; previous < index; ++previous) {
                if ((words[previous] & 0xffu) == semantic) throw std::invalid_argument("AGC: duplicate output semantic");
            }
        }
    }
    const auto offset = writer.Reserve(words.size() * sizeof(ShaderSemantic), alignof(ShaderSemantic));
    for (std::size_t index = 0; index < words.size(); ++index) writer.WriteUInt32(offset + index * sizeof(ShaderSemantic), words[index]);
    writer.WriteRelativePointer(pointerOffset, offset);
}

}

void AgcSemanticBuilder::Build(const PsbcShaderMetadata& metadata, AgcBinaryWriter& writer) {
    if (metadata.input_semantic_count > PSBC_MAX_SEMANTICS || metadata.output_semantic_count > PSBC_MAX_SEMANTICS) throw std::invalid_argument("AGC: too many semantics");
    if ((metadata.source_stage == PSBC_STAGE_VERTEX && metadata.input_semantic_count) || (metadata.source_stage == PSBC_STAGE_FRAGMENT && metadata.output_semantic_count)) throw std::invalid_argument("AGC: semantics do not match the shader stage");
    writeSemantics(writer, {metadata.input_semantics, metadata.input_semantic_count}, offsetof(Shader, input_semantics), false);
    writeSemantics(writer, {metadata.output_semantics, metadata.output_semantic_count}, offsetof(Shader, output_semantics), true);
    writer.WriteUInt32(offsetof(Shader, num_input_semantics), metadata.input_semantic_count);
    writer.WriteUInt16(offsetof(Shader, num_output_semantics), static_cast<std::uint16_t>(metadata.output_semantic_count));
}

}
