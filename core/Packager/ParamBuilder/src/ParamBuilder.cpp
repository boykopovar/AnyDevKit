#include "ParamBuilder.hpp"
#include "Validator/ParamValidator.hpp"
#include "Writer/ParamWriter.hpp"

#include <fstream>

namespace AnyDevKit {

std::string ParamBuilder::Build(const ApplicationParameters& parameters) {
    Param::ParamValidator::Validate(parameters);
    return Param::ParamWriter::Write(parameters);
}

void ParamBuilder::WriteFile(const std::filesystem::path& output, const ApplicationParameters& parameters) {
    const auto text = Build(parameters);
    std::ofstream stream(output, std::ios::binary | std::ios::trunc);
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    stream << text;
    stream.close();
}

}
