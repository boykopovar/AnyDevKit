#include "Commands/NidCommand.hpp"
#include "IO/Files.hpp"
#include <nid/BinaryPatcherFactory.hpp>
#include <nid/ExportExclusions.hpp>
#include <cstring>
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void NidCommand::Run(std::string_view command, Arguments& arguments) {
    const auto input = arguments.Require("input");
    const auto output = arguments.Require("output");
    Files::RequireNew(Files::Path(output));
    if (command == "exports") {
        arguments.Finish();
        if (!Nid::WriteExportList(input, output)) throw std::runtime_error("Cannot write export list");
        return;
    }
    Nid::PatchOptions options;
    options.executable = arguments.Flag("exec");
    options.onlySce = arguments.Flag("only-sce");
    if (options.onlySce && !options.executable) throw std::invalid_argument("--only-sce requires --exec");
    const auto library = options.executable ? arguments.Optional("library") : arguments.Require("library");
    std::unordered_set<std::string> excluded;
    for (const auto& path : arguments.Many("preserve-exports")) {
        const auto names = Nid::ReadExportExclusions(path);
        excluded.insert(names.begin(), names.end());
    }
    for (const auto& path : arguments.Many("preserve-export-list")) {
        const auto names = Nid::ReadExportList(path);
        excluded.insert(names.begin(), names.end());
    }
    arguments.Finish();
    const auto source = Files::Read(Files::Path(input));
    std::vector<std::uint8_t> bytes(source.size());
    if (!bytes.empty()) std::memcpy(bytes.data(), source.data(), source.size());
    const auto patcher = Nid::MakePatcher(bytes);
    if (!patcher) throw std::invalid_argument("Unsupported binary format");
    patcher->PatchNids(bytes, library, excluded, options);
    Files::Write(Files::Path(output), std::as_bytes(std::span(bytes)));
}
}
