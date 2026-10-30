#include "ShaderCompiler.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void writeBytes(std::ostream& output, std::string_view name, std::span<const std::byte> bytes) {
    output << "inline constexpr std::uint8_t " << name << "[]{\n";
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        output << std::to_integer<unsigned>(bytes[i]) << ',';
        output << (i % 24 == 23 ? '\n' : ' ');
    }
    output << "\n};\n";
}

void compile(std::ostream& output, const std::filesystem::path& path, PsbcStage stage, const std::string& name, std::uint32_t constantSize, std::string_view target) {
    auto options = AnyDevKit::ShaderCompiler::CreateOptions(stage, target);
    if (stage == PSBC_STAGE_FRAGMENT) options.spi_shader_col_format = 4;
    const auto shader = AnyDevKit::ShaderCompiler::CompileFile(path, options);
    const auto& metadata = shader.GetBindingMetadata();
    if (metadata.descriptor_binding_count || metadata.vertex_buffer_table_valid || metadata.view_index_valid || metadata.draw_id_valid || metadata.start_instance_valid) throw std::runtime_error("Unexpected calculator shader resource layout");
    std::array<std::uint32_t, 16> data{};
    if (metadata.ngg_lds_layout_valid) data.at(metadata.ngg_lds_layout_user_data_dword) = metadata.ngg_lds_layout;
    if (metadata.push_constants_valid && metadata.push_constant_size > constantSize) throw std::runtime_error("Unexpected calculator push constant size");
    writeBytes(output, name + "Header", shader.GetHeaderBytes());
    writeBytes(output, name + "Code", shader.GetCodeBytes());
    output << "inline constexpr ShaderAsset " << name << '{' << name << "Header, " << name << "Code, {";
    for (const auto value : data) output << value << ", ";
    output << "}, " << metadata.user_sgpr_count << ", " << (metadata.push_constants_valid ? static_cast<int>(metadata.push_constants_user_data_dword) : -1) << "};\n";
}

}

int main(int count, char** arguments) {
    try {
        if (count != 6 && count != 8) throw std::invalid_argument("Expected background vertex/fragment, interface vertex/fragment and output header");
        if (count == 8 && std::string_view(arguments[6]) != "--target") throw std::invalid_argument("Expected --target");
        const std::string_view target = count == 8 ? arguments[7] : "ps5";
        std::ofstream output(arguments[5], std::ios::binary | std::ios::trunc);
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output << "#ifndef ANYDEVKIT_CALCULATOR_GENERATED_SHADERS_HPP\n#define ANYDEVKIT_CALCULATOR_GENERATED_SHADERS_HPP\n#include \"Graphics/ShaderAsset.hpp\"\nnamespace CalculatorShaders {\n";
        compile(output, arguments[1], PSBC_STAGE_VERTEX, "Vertex", 12, target);
        compile(output, arguments[2], PSBC_STAGE_FRAGMENT, "Fragment", 208, target);
        compile(output, arguments[3], PSBC_STAGE_VERTEX, "InterfaceVertex", 64, target);
        compile(output, arguments[4], PSBC_STAGE_FRAGMENT, "InterfaceFragment", 64, target);
        output << "}\n#endif\n";
        output.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
