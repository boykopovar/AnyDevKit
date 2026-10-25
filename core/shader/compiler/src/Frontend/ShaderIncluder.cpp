#include "Frontend/ShaderIncluder.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace AnyDevKit {

ShaderIncluder::ShaderIncluder(std::span<const std::filesystem::path> includeDirectories) {
    for (const auto& directory : includeDirectories) {
        const auto path = std::filesystem::canonical(directory);
        if (!std::filesystem::is_directory(path)) throw std::invalid_argument("GLSL: include path is not a directory: " + path.string());
        this->includeDirectories.push_back(path);
    }
}

std::string ShaderIncluder::ReadFile(const std::filesystem::path& path) {
    std::ifstream input;
    input.exceptions(std::ios::badbit | std::ios::failbit);
    input.open(path, std::ios::binary | std::ios::ate);
    const auto size = input.tellg();
    if (size < 0 || size > std::numeric_limits<int>::max()) throw std::length_error("GLSL: source file exceeds supported length: " + path.string());
    std::string source(static_cast<std::size_t>(size), '\0');
    input.seekg(0);
    if (!source.empty()) input.read(source.data(), static_cast<std::streamsize>(source.size()));
    if (source.find('\0') != std::string::npos) throw std::invalid_argument("GLSL: embedded null byte in source: " + path.string());
    return source;
}

glslang::TShader::Includer::IncludeResult* ShaderIncluder::resolve(const char* headerName, const char* includerName, std::size_t inclusionDepth, bool local) {
    if (!headerName || !*headerName || inclusionDepth > 64) throw std::invalid_argument("GLSL: invalid include name or include depth exceeds 64");
    const std::filesystem::path requested(headerName);
    std::vector<std::filesystem::path> candidates;
    if (requested.is_absolute()) candidates.push_back(requested);
    else {
        if (local && includerName && *includerName) candidates.push_back(std::filesystem::path(includerName).parent_path() / requested);
        for (const auto& directory : includeDirectories) candidates.push_back(directory / requested);
    }
    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate)) continue;
        const auto path = std::filesystem::canonical(candidate);
        if (!std::filesystem::is_regular_file(path)) throw std::invalid_argument("GLSL: include is not a regular file: " + path.string());
        auto source = std::make_unique<std::string>(ReadFile(path));
        auto result = std::make_unique<IncludeResult>(path.generic_string(), source->data(), source->size(), source.get());
        auto* pointer = result.get();
        includes.emplace(pointer, std::make_pair(std::move(source), std::move(result)));
        return pointer;
    }
    throw std::runtime_error("GLSL: include not found: " + requested.string());
}

glslang::TShader::Includer::IncludeResult* ShaderIncluder::includeSystem(const char* headerName, const char* includerName, std::size_t inclusionDepth) { return resolve(headerName, includerName, inclusionDepth, false); }
glslang::TShader::Includer::IncludeResult* ShaderIncluder::includeLocal(const char* headerName, const char* includerName, std::size_t inclusionDepth) { return resolve(headerName, includerName, inclusionDepth, true); }

void ShaderIncluder::releaseInclude(IncludeResult* result) {
    if (!result) return;
    includes.erase(result);
}

}
