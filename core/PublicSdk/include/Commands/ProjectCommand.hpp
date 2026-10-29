#ifndef ANYDEVKIT_PUBLICSDK_PROJECTCOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_PROJECTCOMMAND_HPP
#include "CommandLine/Arguments.hpp"
#include <filesystem>
namespace AnyDevKit::PublicSdk {
class ProjectCommand final {
public:
    static void Init(const std::string& name, Arguments& arguments, const std::filesystem::path& sdkRoot);
    static void Package(Arguments& arguments);
    static void Dependencies(std::string_view command, Arguments& arguments);
};
}
#endif
