#ifndef ANYDEVKIT_PUBLICSDK_NIDCOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_NIDCOMMAND_HPP
#include "CommandLine/Arguments.hpp"
#include <string_view>
namespace AnyDevKit::PublicSdk {
class NidCommand final {
public:
    static void Run(std::string_view command, Arguments& arguments);
};
}
#endif
