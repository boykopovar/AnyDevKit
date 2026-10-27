#include "Writer/ParamWriter.hpp"

#include <string_view>

namespace AnyDevKit::Param {
namespace {

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (char c : value) {
        if (c == '"' || c == '\\') result += '\\';
        result += c;
    }
    return result + '"';
}

std::string hex(std::uint64_t value) {
    std::string result = "0x0000000000000000";
    constexpr std::string_view digits = "0123456789ABCDEF";
    for (std::size_t i = 0; i < 16; ++i) result[17 - i] = digits[(value >> (i * 4)) & 15];
    return quote(result);
}

}

std::string ParamWriter::Write(const ApplicationParameters& parameters) {
    std::string result = "{\n";
    auto field = [&](std::string_view name, const std::string& value) { result += "  " + quote(name) + ": " + value + ",\n"; };
    field("titleId", quote(parameters.TitleId));
    field("contentId", quote(parameters.ContentId));
    field("conceptId", quote(parameters.ConceptId));
    field("contentVersion", quote(parameters.ContentVersion));
    field("masterVersion", quote(parameters.MasterVersion));
    field("applicationCategoryType", std::to_string(parameters.ApplicationCategoryType));
    field("applicationDrmType", quote("standard"));
    field("attribute", std::to_string(parameters.Attribute));
    field("attribute2", std::to_string(parameters.Attribute2));
    field("attribute3", std::to_string(parameters.Attribute3));
    field("requiredSystemSoftwareVersion", hex(parameters.RequiredSystemSoftwareVersion));
    field("sdkVersion", hex(parameters.SdkVersion));
    if (parameters.FlexibleMemorySize) field("kernel", "{\"flexibleMemorySize\": " + std::to_string(*parameters.FlexibleMemorySize) + "}");
    result += "  \"localizedParameters\": {\n    \"defaultLanguage\": " + quote(parameters.DefaultLanguage);
    for (const auto& [language, name] : parameters.Titles) result += ",\n    " + quote(language) + ": {\"titleName\": " + quote(name) + "}";
    return result + "\n  }\n}\n";
}

}
