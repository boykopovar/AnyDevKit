#ifndef ANYDEVKIT_PUBLICSDK_PROJECT_METADATA_HPP
#define ANYDEVKIT_PUBLICSDK_PROJECT_METADATA_HPP
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace AnyDevKit::PublicSdk {
class Metadata final {
public:
    static std::map<std::string, std::vector<std::string>> Read(const std::filesystem::path& path);
};
}
#endif
