#ifndef ANYDEVKIT_PUBLICSDK_SHADERPIPELINECOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_SHADERPIPELINECOMMAND_HPP
#include "CommandLine/Arguments.hpp"
namespace AnyDevKit::PublicSdk {
class ShaderPipelineCommand final {
public:
    static void Run(std::string_view command, Arguments& arguments);
};
}
#endif
