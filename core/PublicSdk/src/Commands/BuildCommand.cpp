#include "Commands/BuildCommand.hpp"
#include "CommandLine/Options.hpp"
#include "IO/Files.hpp"
#include "Pipeline/ApplicationBuilder.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void BuildCommand::Run(std::string_view, Arguments& arguments) {
    ApplicationBuildOptions options;
    options.InputElf = Files::Path(arguments.Require("input"));
    options.App0 = Files::Path(arguments.Require("app0"));
    options.Output = Files::Path(arguments.Require("output"));
    options.Workspace = Files::Path(arguments.Require("workspace"));
    const auto format = arguments.Require("format");
    if (format == "ffpfsc") options.Format = PackageFormat::Ffpfsc;
    else if (format == "pkg") options.Format = PackageFormat::Pkg;
    else throw std::invalid_argument("Supported package formats: ffpfsc, pkg");
    options.Parameters = Options::Parameters(arguments);
    options.Imports = Options::Imports(arguments);
    options.Elf = Options::Elf(arguments);
    options.Eboot = Options::Eboot(arguments);
    if (options.Format == PackageFormat::Pkg) {
        options.Keys = Options::Keys(arguments);
        options.Pfs.Timestamp = Options::Timestamp(arguments);
    } else options.Pfs = Options::Pfs(arguments);
    arguments.Finish();
    ApplicationBuilder::Build(options);
}
}
