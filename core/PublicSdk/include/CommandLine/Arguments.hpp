#ifndef ANYDEVKIT_PUBLICSDK_ARGUMENTS_HPP
#define ANYDEVKIT_PUBLICSDK_ARGUMENTS_HPP
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>
namespace AnyDevKit::PublicSdk {
class Arguments final {
public:
    explicit Arguments(std::span<const std::string> arguments);
    std::string Require(const std::string& name);
    std::string Optional(const std::string& name, const std::string& defaultValue = {});
    std::vector<std::string> Many(const std::string& name);
    bool Flag(const std::string& name);
    void Finish() const;
    static std::uint64_t Number(const std::string& value);
    static std::uint32_t Number32(const std::string& value);
private:
    std::map<std::string, std::vector<std::string>> values;
};
}
#endif
