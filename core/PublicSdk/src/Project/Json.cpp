#include "Project/Json.hpp"
#include "IO/Files.hpp"
#include <stdexcept>
#include <vector>
namespace AnyDevKit::PublicSdk {
nlohmann::json Json::Read(const std::filesystem::path& path) {
    const auto bytes = Files::Read(path);
    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::vector<std::set<std::string>> keys;
    return nlohmann::json::parse(text, [&](int, nlohmann::json::parse_event_t event, nlohmann::json& value) {
        if (event == nlohmann::json::parse_event_t::object_start) keys.emplace_back();
        if (event == nlohmann::json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second) throw std::invalid_argument("Duplicate JSON key: " + value.get<std::string>());
        if (event == nlohmann::json::parse_event_t::object_end) keys.pop_back();
        return true;
    });
}
void Json::Fields(const nlohmann::json& value, const std::set<std::string>& allowed) {
    if (!value.is_object()) throw std::invalid_argument("Expected a JSON object");
    for (const auto& [key, field] : value.items()) {
        static_cast<void>(field);
        if (!allowed.contains(key)) throw std::invalid_argument("Unsupported JSON field: " + key);
    }
}
}
