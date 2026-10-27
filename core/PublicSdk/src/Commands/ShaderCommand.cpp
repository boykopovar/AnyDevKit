#include "Commands/ShaderCommand.hpp"
#include "IO/Files.hpp"
#include "ShaderCompiler.hpp"
#include "Backend/PsbcCompiler.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void ShaderCommand::Run(std::string_view command, Arguments& arguments) {
    const auto input = Files::Path(arguments.Require("input"));
    const auto header = Files::Path(arguments.Require("header"));
    const auto code = Files::Path(arguments.Require("code"));
    Files::RequireNew(header);
    Files::RequireNew(code);
    if (std::filesystem::weakly_canonical(header) == std::filesystem::weakly_canonical(code)) throw std::invalid_argument("Shader header and code outputs must differ");
    const auto stage = arguments.Require("stage");
    if (stage != "vertex" && stage != "fragment") throw std::invalid_argument("Supported stages: vertex, fragment");
    auto options = ShaderCompiler::CreateOptions(stage == "vertex" ? PSBC_STAGE_VERTEX : PSBC_STAGE_FRAGMENT);
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
    Files::Write(header, package.GetHeaderBytes());
    Files::Write(code, package.GetCodeBytes());
}
}
