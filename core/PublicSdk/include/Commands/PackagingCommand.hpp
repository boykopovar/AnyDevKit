#ifndef ANYDEVKIT_PUBLICSDK_PACKAGINGCOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_PACKAGINGCOMMAND_HPP
#include "CommandLine/Arguments.hpp"
#include <string_view>
namespace AnyDevKit::PublicSdk {
class PackagingCommand final {
public:
    static void Run(std::string_view command, Arguments& arguments);
};
}
#endif
