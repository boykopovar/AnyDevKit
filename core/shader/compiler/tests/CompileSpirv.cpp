#include "Backend/PsbcCompiler.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<char> read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open input: " + path.string());
    std::vector<char> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad() || bytes.empty()) throw std::runtime_error("Cannot read input: " + path.string());
    return bytes;
}

std::vector<std::uint32_t> readSpirv(const std::filesystem::path& path) {
    const auto bytes = read(path);
    if (bytes.size() % 4 != 0) throw std::invalid_argument("Invalid SPIR-V size");
    std::vector<std::uint32_t> words(bytes.size() / 4);
    for (std::size_t index = 0; index < bytes.size(); ++index) words[index / 4] |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[index])) << ((index % 4) * 8);
    return words;
}

void compare(const AnyDevKit::AgcShaderPackage& package, const std::filesystem::path& directory, const std::string& stage) {
    const auto header = read(directory / (stage + ".header"));
    const auto code = read(directory / (stage + ".code"));
    const auto actualHeader = package.GetHeaderBytes();
    const auto actualCode = package.GetCodeBytes();
    if (header.size() != actualHeader.size() || std::memcmp(header.data(), actualHeader.data(), header.size()) != 0) throw std::runtime_error("Compiled AGC header differs");
    if (code.size() != actualCode.size() || std::memcmp(code.data(), actualCode.data(), code.size()) != 0) throw std::runtime_error("Compiled AGC code differs");
}

}

int main(int count, char** arguments) {
    try {
        if (count != 2) throw std::invalid_argument("Expected compiled pipeline directory");
        const std::filesystem::path directory(arguments[1]);
        const auto vertex = readSpirv(directory / "Triangle.vert.hlsl.spv");
        const auto geometry = readSpirv(directory / "EmitTriangle.geom.hlsl.spv");
        const auto streamVertex = readSpirv(directory / "VertexStreams.vert.hlsl.spv");
        const auto streamGeometry = readSpirv(directory / "VertexStreams.geom.hlsl.spv");
        const auto control = readSpirv(directory / "Triangle.tesc.hlsl.spv");
        const auto evaluation = readSpirv(directory / "Triangle.tese.hlsl.spv");
        using AnyDevKit::PsbcCompiler;
        for (const auto* target : {"windows", "linux", "ps5"}) {
            const auto package = PsbcCompiler::CompileGeometry(vertex, geometry, PsbcCompiler::CreateOptions(PSBC_STAGE_GEOMETRY, target));
            compare(package, directory / (std::string(target) + "-geometry"), "geometry");
            auto dynamicOptions = PsbcCompiler::CreateOptions(PSBC_STAGE_GEOMETRY, target);
            dynamicOptions.dynamic_vertex_input = true;
            const auto dynamicPackage = PsbcCompiler::CompileGeometry(streamVertex, streamGeometry, dynamicOptions);
            const auto& dynamicMetadata = dynamicPackage.GetBindingMetadata();
            if (!dynamicMetadata.dynamic_vertex_input || !dynamicMetadata.vertex_buffer_per_attribute || dynamicMetadata.vertex_buffer_descriptor_stride != 32 || dynamicMetadata.vertex_buffer_usage_mask != 3) throw std::runtime_error("Dynamic geometry input layout was lost");
            auto options = PsbcCompiler::CreateOptions(PSBC_STAGE_TESS_CTRL, target);
            options.patch_control_points = 3;
            const auto tessellation = PsbcCompiler::CompileTessellation(vertex, control, evaluation, options);
            compare(tessellation[0], directory / (std::string(target) + "-tessellation"), "control");
            compare(tessellation[1], directory / (std::string(target) + "-tessellation"), "evaluation");
        }
        std::cout << "Offline SPIR-V compiler matches all pipeline outputs\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
