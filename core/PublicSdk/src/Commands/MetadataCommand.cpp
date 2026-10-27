#include "Commands/MetadataCommand.hpp"
#include "CommandLine/Options.hpp"
#include "IO/Files.hpp"
#include "IconBuilder.hpp"
#include "WindowsStartupBuilder.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void MetadataCommand::Run(std::string_view command, Arguments& arguments) {
    if (command != "param" && command != "png" && command != "dds" && command != "windows-startup") throw std::invalid_argument("Unsupported MetadataCommand command");
    const auto output = Files::Path(arguments.Require("output"));
    Files::RequireNew(output);
    if (command == "param") {
        const auto parameters = Options::Parameters(arguments);
        arguments.Finish();
        ParamBuilder::WriteFile(output, parameters);
    } else if (command == "windows-startup") {
        arguments.Finish();
        WindowsStartupBuilder::WriteFile(output);
    } else {
        const auto input = Files::Path(arguments.Require("input"));
        arguments.Finish();
        const auto image = IconBuilder::ReadPng(input);
        if (command == "dds") IconBuilder::WriteDds(output, image);
        else IconBuilder::WritePng(output, image);
    }
}
}
