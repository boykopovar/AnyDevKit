#ifndef ANYDEVKIT_PUBLICSDK_METADATACOMMAND_HPP
#define ANYDEVKIT_PUBLICSDK_METADATACOMMAND_HPP
#include "CommandLine/Arguments.hpp"
#include <string_view>
namespace AnyDevKit::PublicSdk {
class MetadataCommand final {
public:
    static void Run(std::string_view command, Arguments& arguments);
};
}
#endif
