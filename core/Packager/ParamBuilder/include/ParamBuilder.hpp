#ifndef ANYDEVKIT_PARAM_BUILDER_HPP
#define ANYDEVKIT_PARAM_BUILDER_HPP

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace AnyDevKit {

struct ApplicationParameters {
    std::string TitleId;
    std::string ContentId;
    std::string ConceptId;
    std::string ContentVersion;
    std::string MasterVersion;
    std::string DefaultLanguage;
    std::map<std::string, std::string> Titles;
    std::uint64_t RequiredSystemSoftwareVersion = 0;
    std::uint64_t SdkVersion = 0;
    std::uint32_t ApplicationCategoryType = 0;
    std::uint32_t Attribute = 0;
    std::uint32_t Attribute2 = 0;
    std::uint32_t Attribute3 = 0;
    std::optional<std::uint64_t> FlexibleMemorySize;
};

class ParamBuilder final {
public:
    static std::string Build(const ApplicationParameters& parameters);
    static void WriteFile(const std::filesystem::path& output, const ApplicationParameters& parameters);
};

}

#endif
