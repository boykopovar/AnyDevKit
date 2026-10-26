#include "Validator/ParamValidator.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

namespace AnyDevKit::Param {
namespace {

bool digits(std::string_view value) {
    return !value.empty() && std::ranges::all_of(value, [](char c) { return c >= '0' && c <= '9'; });
}

bool uppercase(std::string_view value) {
    return std::ranges::all_of(value, [](char c) { return c >= 'A' && c <= 'Z'; });
}

void validateTitle(std::string_view value) {
    if (value.empty()) throw std::invalid_argument("Title name is empty");
    for (std::size_t i = 0; i < value.size();) {
        const auto first = static_cast<unsigned char>(value[i++]);
        if (first < 0x20 || first == 0x7f) throw std::invalid_argument("Title name contains a control character");
        if (first < 0x80) continue;
        const unsigned count = first >= 0xc2 && first <= 0xdf ? 1 : first >= 0xe0 && first <= 0xef ? 2 : first >= 0xf0 && first <= 0xf4 ? 3 : 0;
        if (count == 0 || count > value.size() - i) throw std::invalid_argument("Title name is not valid UTF-8");
        std::uint32_t point = first & (0x7f >> (count + 1));
        for (unsigned j = 0; j < count; ++j) {
            const auto next = static_cast<unsigned char>(value[i++]);
            if ((next & 0xc0) != 0x80) throw std::invalid_argument("Title name is not valid UTF-8");
            point = (point << 6) | (next & 0x3f);
        }
        if ((count == 1 && point < 0x80) || (count == 2 && point < 0x800) || (count == 3 && point < 0x10000) || point > 0x10ffff || (point >= 0xd800 && point <= 0xdfff)) throw std::invalid_argument("Title name is not valid UTF-8");
    }
}

}

void ParamValidator::Validate(const ApplicationParameters& parameters) {
    const std::string_view title = parameters.TitleId;
    if (title.size() != 9 || !uppercase(title.substr(0, 4)) || !digits(title.substr(4))) throw std::invalid_argument("Title ID must contain four uppercase letters and five digits");
    const std::string_view content = parameters.ContentId;
    if (content.size() != 36 || !uppercase(content.substr(0, 2)) || !digits(content.substr(2, 4)) || content[6] != '-' || content.substr(7, 9) != title || content[16] != '_' || !digits(content.substr(17, 2)) || content[19] != '-' || !std::ranges::all_of(content.substr(20), [](char c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; })) throw std::invalid_argument("Content ID has invalid syntax or a different title ID");
    if (parameters.ConceptId.size() != 8 || !digits(parameters.ConceptId)) throw std::invalid_argument("Concept ID must contain eight digits");
    const std::string_view version = parameters.ContentVersion;
    if (version.size() != 10 || version[2] != '.' || version[6] != '.' || !digits(version.substr(0, 2)) || !digits(version.substr(3, 3)) || !digits(version.substr(7, 3))) throw std::invalid_argument("Content version must use NN.NNN.NNN");
    const std::string_view master = parameters.MasterVersion;
    if (master.size() != 5 || master[2] != '.' || !digits(master.substr(0, 2)) || !digits(master.substr(3, 2))) throw std::invalid_argument("Master version must use NN.NN");
    if (!parameters.Titles.contains(parameters.DefaultLanguage)) throw std::invalid_argument("Default language has no title name");
    constexpr std::array<std::string_view, 30> languages{"ar-AE", "cs-CZ", "da-DK", "de-DE", "el-GR", "en-GB", "en-US", "es-ES", "es-419", "fi-FI", "fr-CA", "fr-FR", "hu-HU", "id-ID", "it-IT", "ja-JP", "ko-KR", "nl-NL", "no-NO", "pl-PL", "pt-BR", "pt-PT", "ro-RO", "ru-RU", "sv-SE", "th-TH", "tr-TR", "uk-UA", "vi-VN", "zh-Hans"};
    for (const auto& [language, name] : parameters.Titles) {
        if (language != "zh-Hant" && std::ranges::find(languages, language) == languages.end()) throw std::invalid_argument("Unsupported PS5 title language: " + language);
        validateTitle(name);
    }
    if (parameters.RequiredSystemSoftwareVersion == 0 || parameters.SdkVersion == 0) throw std::invalid_argument("Firmware and SDK versions must be specified");
    if (parameters.FlexibleMemorySize && (*parameters.FlexibleMemorySize == 0 || *parameters.FlexibleMemorySize % 0x4000 != 0)) throw std::invalid_argument("Flexible memory size must be a positive multiple of 16 KiB");
}

}
