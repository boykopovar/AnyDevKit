#include "Commands/ShaderPipelineCommand.hpp"
#include "ShaderCompiler.hpp"
#include "CommandLine/Options.hpp"
#include "Shader/ShaderMetadata.hpp"
#include "IO/Files.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
namespace {
std::string read(const std::filesystem::path& path) {
    const auto bytes = Files::Read(path);
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}
void write(const std::filesystem::path& directory, const std::string& stage, const AgcShaderPackage& package) {
    const auto metadata = ShaderMetadata::Encode(package.GetBindingMetadata());
    Files::Write(directory / (stage + ".header"), package.GetHeaderBytes());
    Files::Write(directory / (stage + ".code"), package.GetCodeBytes());
    Files::Write(directory / (stage + ".adk"), std::as_bytes(std::span(metadata)));
}
}
void ShaderPipelineCommand::Run(std::string_view command, Arguments& arguments) {
    if (command != "shader-pipeline") throw std::invalid_argument("Unknown pipeline command");
    const auto vertexPath = arguments.Require("vertex");
    const auto geometryPath = arguments.Optional("geometry");
    const auto controlPath = arguments.Optional("control");
    const auto evaluationPath = arguments.Optional("evaluation");
    const auto output = Files::Path(arguments.Require("output"));
    Files::RequireNew(output);
    const bool geometry = !geometryPath.empty();
    if (geometry ? !controlPath.empty() || !evaluationPath.empty() : controlPath.empty() || evaluationPath.empty()) throw std::invalid_argument("Specify vertex + geometry or vertex + control + evaluation");
    auto options = Options::Shader(arguments, geometry ? PSBC_STAGE_GEOMETRY : PSBC_STAGE_TESS_CTRL);
    const auto entry = arguments.Optional("entry", "main");
    options.entrypoint = entry.c_str();
    options.address32_hi = Arguments::Number32(arguments.Optional("address32-hi", "2"));
    options.primitive_type = Arguments::Number32(arguments.Optional("primitive-type", "4"));
    if (!geometry) options.patch_control_points = Arguments::Number32(arguments.Require("patch-control-points"));
    std::vector<std::filesystem::path> includes;
    for (const auto& path : arguments.Many("include")) includes.push_back(Files::Path(path));
    arguments.Finish();
    const auto vertex = read(Files::Path(vertexPath));
    if (geometry) {
        const auto source = read(Files::Path(geometryPath));
        const auto package = ShaderCompiler::CompileGeometry({vertex, vertexPath}, {source, geometryPath}, options, includes);
        std::filesystem::create_directory(output);
        write(output, "geometry", package);
    } else {
        const auto control = read(Files::Path(controlPath));
        const auto evaluation = read(Files::Path(evaluationPath));
        const auto packages = ShaderCompiler::CompileTessellation({vertex, vertexPath}, {control, controlPath}, {evaluation, evaluationPath}, options, includes);
        std::filesystem::create_directory(output);
        write(output, "control", packages[0]);
        write(output, "evaluation", packages[1]);
    }
}
}
