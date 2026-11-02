#ifndef ANYDEVKIT_PUBLICSDK_SETUPCOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_SETUPCOMMAND_HPP
#include <filesystem>
#include <span>
#include <string>
namespace AnyDevKit::PublicSdk {
class SetupCommand final {
public:
    static void Run(std::span<const std::string> arguments, const std::filesystem::path& sdkRoot);
};
}
#endif
