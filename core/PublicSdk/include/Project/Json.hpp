#ifndef ANYDEVKIT_PUBLICSDK_PROJECT_JSON_HPP
#define ANYDEVKIT_PUBLICSDK_PROJECT_JSON_HPP
#include <nlohmann/json.hpp>
#include <filesystem>
#include <set>
namespace AnyDevKit::PublicSdk {
class Json final {
public:
    static nlohmann::json Read(const std::filesystem::path& path);
    static void Fields(const nlohmann::json& value, const std::set<std::string>& allowed);
};
}
#endif
