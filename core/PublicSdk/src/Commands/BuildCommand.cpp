#include "Commands/BuildCommand.hpp"
#include "CommandLine/Options.hpp"
#include "IO/Files.hpp"
#include "Pipeline/ApplicationBuilder.hpp"
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void BuildCommand::Run(std::string_view command, Arguments& arguments) {
    if (command != "build") throw std::invalid_argument("Unsupported build command");
    ApplicationBuildOptions options;
    options.InputElf = Files::Path(arguments.Require("input"));
    options.App0 = Files::Path(arguments.Require("app0"));
    options.Output = Files::Path(arguments.Require("output"));
    options.Workspace = Files::Path(arguments.Require("workspace"));
    const auto format = arguments.Require("format");
    if (format == "ffpfsc") options.Format = PackageFormat::Ffpfsc;
    else if (format == "pkg") options.Format = PackageFormat::Pkg;
    else throw std::invalid_argument("Supported package formats: ffpfsc, pkg");
    const auto paramPath = arguments.Optional("param");
    if (!paramPath.empty()) {
        arguments.MergeDefaults({{"param", {paramPath}}});
        const auto bytes = Files::Read(Files::Path(paramPath));
        Arguments original(std::vector<std::string>{"--param", paramPath});
        const auto parameters = Options::Parameters(original);
        original.Finish();
        options.Parameters = Options::Parameters(arguments);
        if (ParamBuilder::Build(parameters) == ParamBuilder::Build(options.Parameters)) options.ParameterJson = std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        else options.ParameterJson = ParamBuilder::Build(options.Parameters);
    } else options.Parameters = Options::Parameters(arguments);
    options.Imports = Options::Imports(arguments, options.InputElf);
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
