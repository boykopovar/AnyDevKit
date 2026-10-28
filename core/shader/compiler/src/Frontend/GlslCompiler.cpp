#include "Frontend/GlslCompiler.hpp"

#include "Frontend/ShaderIncluder.hpp"
#include "Validator/ShaderLayoutValidator.hpp"
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

namespace AnyDevKit {

namespace {

class GlslangRuntime {
public:
    GlslangRuntime() {
        if (!glslang::InitializeProcess()) throw std::runtime_error("GLSL: initialization failed");
    }
    ~GlslangRuntime() { glslang::FinalizeProcess(); }
    GlslangRuntime(const GlslangRuntime&) = delete;
    GlslangRuntime& operator=(const GlslangRuntime&) = delete;
};

}

std::vector<std::uint32_t> GlslCompiler::Compile(std::string_view source, std::string_view sourceName, const PsbcCompileOptions& compileOptions, std::span<const std::filesystem::path> includeDirectories) {
    const auto stage = compileOptions.stage;
    if (source.empty() || source.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) || source.find('\0') != std::string_view::npos) throw std::invalid_argument("GLSL: empty, oversized or null-containing source");
    if (sourceName.empty() || sourceName.find('\0') != std::string_view::npos) throw std::invalid_argument("GLSL: invalid source name");
    if (stage != PSBC_STAGE_VERTEX && stage != PSBC_STAGE_FRAGMENT && stage != PSBC_STAGE_GEOMETRY) throw std::invalid_argument("GLSL: unsupported AGC stage");
    static std::mutex mutex;
    const std::lock_guard lock(mutex);
    static const GlslangRuntime runtime;
    const auto language = stage == PSBC_STAGE_VERTEX ? EShLangVertex : stage == PSBC_STAGE_GEOMETRY ? EShLangGeometry : EShLangFragment;
    glslang::TShader shader(language);
    const auto* text = source.data();
    const auto length = static_cast<int>(source.size());
    const std::string name(sourceName);
    const auto* namePointer = name.c_str();
    shader.setStringsWithLengthsAndNames(&text, &length, &namePointer, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, language, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    shader.setEntryPoint("main");
    shader.setAutoMapBindings(false);
    shader.setAutoMapLocations(false);
    ShaderIncluder includer(includeDirectories);
    constexpr auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages, includer)) throw std::runtime_error("GLSL: " + name + "\n" + shader.getInfoLog() + shader.getInfoDebugLog());
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) throw std::runtime_error("GLSL link: " + name + "\n" + program.getInfoLog() + program.getInfoDebugLog());
    ShaderLayoutValidator::Validate(program, compileOptions);
    const auto* intermediate = program.getIntermediate(language);
    if (!intermediate) throw std::runtime_error("GLSL: linker produced no intermediate representation");
    glslang::SpvOptions options;
    options.disableOptimizer = true;
    options.validate = false;
    spv::SpvBuildLogger logger;
    std::vector<std::uint32_t> spirv;
    glslang::GlslangToSpv(*intermediate, spirv, &logger, &options);
    const auto diagnostics = logger.getAllMessages();
    if (!diagnostics.empty()) throw std::runtime_error("SPIR-V: " + name + "\n" + diagnostics);
    if (spirv.size() < 5 || spirv.front() != 0x07230203u) throw std::runtime_error("GLSL: invalid generated SPIR-V module");
    return spirv;
}

}
