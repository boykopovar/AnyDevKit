#include "Commands/PackagingCommand.hpp"
#include "CommandLine/Options.hpp"
#include "IO/Files.hpp"
#include "FfpfscPackager.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void PackagingCommand::Run(std::string_view command, Arguments& arguments) {
    if (command != "ps5-elf" && command != "eboot" && command != "pfs" && command != "ffpfsc" && command != "pkg") throw std::invalid_argument("Unsupported PackagingCommand command");
    const auto input = Files::Path(arguments.Require("input"));
    const auto output = Files::Path(arguments.Require("output"));
    Files::RequireNew(output);
    if (command == "ps5-elf") {
        const auto options = Options::Elf(arguments);
        const auto imports = Options::Imports(arguments);
        arguments.Finish();
        Ps5ElfBuilder::BuildFile(input, output, imports, options);
    } else if (command == "eboot") {
        const auto options = Options::Eboot(arguments);
        arguments.Finish();
        EbootPackager::PackFile(input, output, options);
    } else if (command == "pfs") {
        const auto options = Options::Pfs(arguments);
        arguments.Finish();
        Files::RequireOutside(output, input);
        PfsBuilder::Build(PfsBuilder::ScanDirectory(input), output, options);
    } else {
        const auto workspace = Files::Path(arguments.Require("workspace"));
        Files::RequireNew(workspace);
        if (command == "ffpfsc") {
            const auto options = Options::Pfs(arguments);
            arguments.Finish();
            FfpfscPackager::PackDirectory(input, output, workspace, options);
        } else {
            PkgOptions options;
            options.Parameters = Options::Parameters(arguments);
            options.Keys = Options::Keys(arguments);
            options.Timestamp = Options::Timestamp(arguments);
            arguments.Finish();
            PkgPackager::PackDirectory(input, output, workspace, options);
        }
    }
}
}
