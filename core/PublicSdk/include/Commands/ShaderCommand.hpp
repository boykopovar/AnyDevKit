#ifndef ANYDEVKIT_PUBLICSDK_SHADERCOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_SHADERCOMMAND_HPP
#include "CommandLine/Arguments.hpp"
#include <string_view>
namespace AnyDevKit::PublicSdk {
class ShaderCommand final {
public:
    static void Run(std::string_view command, Arguments& arguments);
};
}
#endif
