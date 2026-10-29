#include "Application.hpp"
#include "CommandLine/Help.hpp"
#include "CommandLine/Arguments.hpp"
#include "Commands/BuildCommand.hpp"
#include "Commands/ProjectCommand.hpp"
#include "Commands/MetadataCommand.hpp"
#include "Commands/NidCommand.hpp"
#include "Commands/PackagingCommand.hpp"
#include "Commands/ShaderCommand.hpp"
#include "Commands/ShaderPipelineCommand.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void Application::Run(std::span<const std::string> arguments, const std::filesystem::path& sdkRoot) {
    if (arguments.empty() || (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "help"))) {
        Help::Print({});
        return;
    }
    if (arguments.size() == 2 && arguments[1] == "--help") { Help::Print(arguments.front()); return; }
    if (arguments.front() == "init") {
        if (arguments.size() < 2) throw std::invalid_argument("Expected adk init Name");
        Arguments options(arguments.subspan(2));
        ProjectCommand::Init(arguments[1], options, sdkRoot);
        return;
    }
    if (arguments.front() == "package" || arguments.front() == "setup" || arguments.front() == "update") {
        Arguments options(arguments.subspan(1));
        if (arguments.front() == "package") ProjectCommand::Package(options);
        else ProjectCommand::Dependencies(arguments.front(), options);
        return;
    }
    using Handler = void (*)(std::string_view, Arguments&);
    const std::map<std::string, Handler> commands{
        {"build", BuildCommand::Run}, {"nid", NidCommand::Run}, {"exports", NidCommand::Run},
        {"ps5-elf", PackagingCommand::Run}, {"eboot", PackagingCommand::Run},
        {"pfs", PackagingCommand::Run}, {"ffpfsc", PackagingCommand::Run}, {"pkg", PackagingCommand::Run},
        {"shader-pipeline", ShaderPipelineCommand::Run}, {"param", MetadataCommand::Run}, {"png", MetadataCommand::Run}, {"dds", MetadataCommand::Run},
        {"windows-startup", MetadataCommand::Run}, {"shader", ShaderCommand::Run}, {"agc", ShaderCommand::Run}, {"spirv", ShaderCommand::Run}
    };
    const auto found = commands.find(arguments.front());
    if (found == commands.end()) throw std::invalid_argument("Unknown command: " + arguments.front());
    Arguments options(arguments.subspan(1));
    found->second(arguments.front(), options);
}
}
