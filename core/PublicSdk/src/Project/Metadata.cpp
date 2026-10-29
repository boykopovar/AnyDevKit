#include "Project/Metadata.hpp"
#include "Project/Json.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
std::map<std::string, std::vector<std::string>> Metadata::Read(const std::filesystem::path& path) {
    const auto value = Json::Read(path);
    const std::map<std::string, std::string> strings{{"titleId", "title-id"}, {"contentId", "content-id"}, {"conceptId", "concept-id"}, {"contentVersion", "content-version"}, {"masterVersion", "master-version"}, {"requiredSystemSoftwareVersion", "firmware-version"}, {"sdkVersion", "sdk-version"}};
    const std::map<std::string, std::string> numbers{{"applicationCategoryType", "category"}, {"attribute", "attribute"}, {"attribute2", "attribute2"}, {"attribute3", "attribute3"}};
    std::set<std::string> allowed{"applicationDrmType", "localizedParameters", "kernel"};
    for (const auto& [key, option] : strings) { static_cast<void>(option); allowed.insert(key); }
    for (const auto& [key, option] : numbers) { static_cast<void>(option); allowed.insert(key); }
    Json::Fields(value, allowed);
    if (value.at("applicationDrmType").get<std::string>() != "standard") throw std::invalid_argument("Unsupported applicationDrmType");
    std::map<std::string, std::vector<std::string>> result;
    for (const auto& [key, option] : strings) result[option] = {value.at(key).get<std::string>()};
    for (const auto& [key, option] : numbers) {
        const auto& number = value.at(key);
        if (!number.is_number_unsigned()) throw std::invalid_argument("Expected an unsigned integer: " + key);
        result[option] = {number.dump()};
    }
    const auto& localized = value.at("localizedParameters");
    if (!localized.is_object()) throw std::invalid_argument("Expected localizedParameters object");
    result["default-language"] = {localized.at("defaultLanguage").get<std::string>()};
    for (const auto& [language, title] : localized.items()) {
        if (language == "defaultLanguage") continue;
        Json::Fields(title, {"titleName"});
        result["title"].push_back(language + "=" + title.at("titleName").get<std::string>());
    }
    if (value.contains("kernel")) {
        const auto& kernel = value.at("kernel");
        Json::Fields(kernel, {"flexibleMemorySize"});
        if (!kernel.at("flexibleMemorySize").is_number_unsigned()) throw std::invalid_argument("Expected unsigned flexibleMemorySize");
        result["flexible-memory"] = {kernel.at("flexibleMemorySize").dump()};
    }
    return result;
}
}
