#include "Commands/ShaderCommand.hpp"
#include "IO/Files.hpp"
#include "ShaderCompiler.hpp"
#include "Shader/ShaderMetadata.hpp"
#include "Backend/PsbcCompiler.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void ShaderCommand::Run(std::string_view command, Arguments& arguments) {
    if (command != "shader" && command != "spirv" && command != "agc") throw std::invalid_argument("Unsupported ShaderCommand command");
    const auto input = Files::Path(arguments.Require("input"));
    const auto header = Files::Path(arguments.Require("header"));
    const auto code = Files::Path(arguments.Require("code"));
    Files::RequireNew(header);
    Files::RequireNew(code);
    if (std::filesystem::weakly_canonical(header) == std::filesystem::weakly_canonical(code)) throw std::invalid_argument("Shader header and code outputs must differ");
    const auto metadataPath = Files::Path(arguments.Require("metadata"));
    if (command == "agc") {
        arguments.Finish();
        const auto metadataBytes = Files::Read(metadataPath);
        if (metadataBytes.empty()) throw std::invalid_argument("Empty shader metadata");
        const std::string_view text(reinterpret_cast<const char*>(metadataBytes.data()), metadataBytes.size());
        auto bytes = Files::Read(input);
        PsbcShaderOutput compiled{};
        compiled.metadata = ShaderMetadata::Decode(text);
        compiled.machine_code = bytes.data();
        compiled.machine_code_size = bytes.size();
        const auto package = AgcPackager::Build(compiled);
        Files::Write(header, package.GetHeaderBytes());
        Files::Write(code, package.GetCodeBytes());
        return;
    }
    Files::RequireNew(metadataPath);
    if (std::filesystem::weakly_canonical(metadataPath) == std::filesystem::weakly_canonical(header) || std::filesystem::weakly_canonical(metadataPath) == std::filesystem::weakly_canonical(code)) throw std::invalid_argument("Shader outputs must differ");
    const auto stage = arguments.Require("stage");
    if (stage != "vertex" && stage != "fragment" && stage != "compute") throw std::invalid_argument("Supported stages: vertex, fragment, compute");
    auto options = ShaderCompiler::CreateOptions(stage == "vertex" ? PSBC_STAGE_VERTEX : stage == "fragment" ? PSBC_STAGE_FRAGMENT : PSBC_STAGE_COMPUTE);
    const auto entry = arguments.Optional("entry", "main");
    options.entrypoint = entry.c_str();
    options.address32_hi = Arguments::Number32(arguments.Optional("address32-hi", "2"));
    options.primitive_type = Arguments::Number32(arguments.Optional("primitive-type", "4"));
    options.rasterization_samples = Arguments::Number32(arguments.Optional("samples", "1"));
    options.spi_shader_col_format = Arguments::Number32(arguments.Optional("color-format", stage == "fragment" ? "9" : "0"));
    std::vector<std::filesystem::path> includes;
    if (command == "shader") for (const auto& path : arguments.Many("include")) includes.push_back(Files::Path(path));
    arguments.Finish();
    const auto compile = [&]() {
        if (command == "shader") return ShaderCompiler::CompileFile(input, options, includes);
        const auto bytes = Files::Read(input);
        if (bytes.empty() || bytes.size() % 4 != 0) throw std::invalid_argument("SPIR-V size must be a nonzero multiple of four");
        std::vector<std::uint32_t> words(bytes.size() / 4);
        for (std::size_t i = 0; i < words.size(); ++i) for (unsigned j = 0; j < 4; ++j) words[i] |= std::to_integer<std::uint32_t>(bytes[4 * i + j]) << (8 * j);
        return PsbcCompiler::Compile(words, options);
    };
    const auto package = compile();
    const auto metadata = ShaderMetadata::Encode(package.GetBindingMetadata());
    Files::Write(header, package.GetHeaderBytes());
    Files::Write(code, package.GetCodeBytes());
    Files::Write(metadataPath, std::as_bytes(std::span(metadata)));
}
}
