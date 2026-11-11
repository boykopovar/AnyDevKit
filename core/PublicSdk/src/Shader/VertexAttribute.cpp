#include "Shader/VertexAttribute.hpp"
#include "CommandLine/Arguments.hpp"
#include <array>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>

namespace AnyDevKit::PublicSdk {

PsbcVertexAttribute VertexAttribute::Parse(const std::string& value) {
    std::array<std::string, 7> fields;
    std::size_t begin = 0;
    for (std::size_t index = 0; index < fields.size(); ++index) {
        const auto end = value.find(',', begin);
        if ((end == std::string::npos) != (index == fields.size() - 1)) throw std::invalid_argument("Expected --vertex-attribute location,binding,format,offset,stride,alignment,divisor");
        fields[index] = value.substr(begin, end == std::string::npos ? end : end - begin);
        if (fields[index].empty()) throw std::invalid_argument("Empty vertex attribute field");
        begin = end == std::string::npos ? value.size() : end + 1;
    }
    static const std::map<std::string_view, PsbcVertexFormat> formats{
        {"r32_float", PSBC_VERTEX_FORMAT_R32_FLOAT},
        {"r32g32_float", PSBC_VERTEX_FORMAT_R32G32_FLOAT},
        {"r32g32b32_float", PSBC_VERTEX_FORMAT_R32G32B32_FLOAT},
        {"r32g32b32a32_float", PSBC_VERTEX_FORMAT_R32G32B32A32_FLOAT},
        {"b8g8r8a8_unorm", PSBC_VERTEX_FORMAT_B8G8R8A8_UNORM},
        {"r10g10b10a2_unorm", PSBC_VERTEX_FORMAT_R10G10B10A2_UNORM},
        {"b10g10r10a2_unorm", PSBC_VERTEX_FORMAT_B10G10R10A2_UNORM},
        {"r10g10b10a2_snorm", PSBC_VERTEX_FORMAT_R10G10B10A2_SNORM},
        {"b10g10r10a2_snorm", PSBC_VERTEX_FORMAT_B10G10R10A2_SNORM},
        {"r10g10b10a2_uscaled", PSBC_VERTEX_FORMAT_R10G10B10A2_USCALED},
        {"b10g10r10a2_uscaled", PSBC_VERTEX_FORMAT_B10G10R10A2_USCALED},
        {"r10g10b10a2_sscaled", PSBC_VERTEX_FORMAT_R10G10B10A2_SSCALED},
        {"b10g10r10a2_sscaled", PSBC_VERTEX_FORMAT_B10G10R10A2_SSCALED},
        {"r32_sint", PSBC_VERTEX_FORMAT_R32_SINT},
        {"r32g32_sint", PSBC_VERTEX_FORMAT_R32G32_SINT},
        {"r32g32b32_sint", PSBC_VERTEX_FORMAT_R32G32B32_SINT},
        {"r32g32b32a32_sint", PSBC_VERTEX_FORMAT_R32G32B32A32_SINT},
        {"r32_uint", PSBC_VERTEX_FORMAT_R32_UINT},
        {"r32g32_uint", PSBC_VERTEX_FORMAT_R32G32_UINT},
        {"r32g32b32_uint", PSBC_VERTEX_FORMAT_R32G32B32_UINT},
        {"r32g32b32a32_uint", PSBC_VERTEX_FORMAT_R32G32B32A32_UINT},
        {"r8g8b8a8_unorm", PSBC_VERTEX_FORMAT_R8G8B8A8_UNORM},
        {"r8_unorm", PSBC_VERTEX_FORMAT_R8_UNORM},
        {"r8_snorm", PSBC_VERTEX_FORMAT_R8_SNORM},
        {"r8_uint", PSBC_VERTEX_FORMAT_R8_UINT},
        {"r8_sint", PSBC_VERTEX_FORMAT_R8_SINT},
        {"r8g8_unorm", PSBC_VERTEX_FORMAT_R8G8_UNORM},
        {"r8g8_snorm", PSBC_VERTEX_FORMAT_R8G8_SNORM},
        {"r8g8_uint", PSBC_VERTEX_FORMAT_R8G8_UINT},
        {"r8g8_sint", PSBC_VERTEX_FORMAT_R8G8_SINT},
        {"r8g8b8a8_snorm", PSBC_VERTEX_FORMAT_R8G8B8A8_SNORM},
        {"r8g8b8a8_uint", PSBC_VERTEX_FORMAT_R8G8B8A8_UINT},
        {"r8g8b8a8_sint", PSBC_VERTEX_FORMAT_R8G8B8A8_SINT},
        {"r16_unorm", PSBC_VERTEX_FORMAT_R16_UNORM},
        {"r16_snorm", PSBC_VERTEX_FORMAT_R16_SNORM},
        {"r16_uint", PSBC_VERTEX_FORMAT_R16_UINT},
        {"r16_sint", PSBC_VERTEX_FORMAT_R16_SINT},
        {"r16_float", PSBC_VERTEX_FORMAT_R16_FLOAT},
        {"r16g16_unorm", PSBC_VERTEX_FORMAT_R16G16_UNORM},
        {"r16g16_snorm", PSBC_VERTEX_FORMAT_R16G16_SNORM},
        {"r16g16_uint", PSBC_VERTEX_FORMAT_R16G16_UINT},
        {"r16g16_sint", PSBC_VERTEX_FORMAT_R16G16_SINT},
        {"r16g16_float", PSBC_VERTEX_FORMAT_R16G16_FLOAT},
        {"r16g16b16a16_unorm", PSBC_VERTEX_FORMAT_R16G16B16A16_UNORM},
        {"r16g16b16a16_snorm", PSBC_VERTEX_FORMAT_R16G16B16A16_SNORM},
        {"r16g16b16a16_uint", PSBC_VERTEX_FORMAT_R16G16B16A16_UINT},
        {"r16g16b16a16_sint", PSBC_VERTEX_FORMAT_R16G16B16A16_SINT},
        {"r16g16b16a16_float", PSBC_VERTEX_FORMAT_R16G16B16A16_FLOAT},
    };
    const auto format = formats.find(fields[2]);
    if (format == formats.end()) throw std::invalid_argument("Unknown vertex format: " + fields[2]);
    const auto location = Arguments::Number32(fields[0]);
    const auto binding = Arguments::Number32(fields[1]);
    if (location > std::numeric_limits<std::uint8_t>::max() || binding > std::numeric_limits<std::uint8_t>::max()) throw std::out_of_range("Vertex location or binding exceeds eight bits");
    return {static_cast<std::uint8_t>(location), static_cast<std::uint8_t>(binding), format->second, Arguments::Number32(fields[3]), Arguments::Number32(fields[4]), Arguments::Number32(fields[5]), Arguments::Number32(fields[6])};
}

}
