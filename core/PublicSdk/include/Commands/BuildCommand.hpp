#ifndef ANYDEVKIT_PUBLICSDK_BUILDCOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_BUILDCOMMAND_HPP
#include "CommandLine/Arguments.hpp"
#include <string_view>
namespace AnyDevKit::PublicSdk {
class BuildCommand final {
public:
    static void Run(std::string_view command, Arguments& arguments);
};
}
#endif
