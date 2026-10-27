#include "Pipeline/ApplicationBuilder.hpp"
#include "IO/Files.hpp"
#include "FfpfscPackager.hpp"
#include "IconBuilder.hpp"
#include "KeystoneBuilder.hpp"
#include <set>
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void ApplicationBuilder::Build(const ApplicationBuildOptions& options) {
    if (options.Format != PackageFormat::Ffpfsc && options.Format != PackageFormat::Pkg) throw std::invalid_argument("Unsupported package format");
    if ((options.Format == PackageFormat::Pkg) != options.Keys.has_value()) throw std::invalid_argument("PKG requires package keys; FFPFSC does not accept them");
    Files::RequireNew(options.Output);
    Files::RequireNew(options.Workspace);
    Files::RequireOutside(options.Output, options.App0);
    Files::RequireOutside(options.Workspace, options.App0);
    const auto workspace = std::filesystem::weakly_canonical(options.Workspace);
    const auto output = std::filesystem::weakly_canonical(options.Output);
    const auto relative = output.lexically_relative(workspace);
    if (output == workspace || (!relative.empty() && *relative.begin() != "..")) throw std::invalid_argument("Package output must be outside workspace");
    const auto entries = PfsBuilder::ScanDirectory(options.App0);
    const std::set<std::string> generated{"eboot.bin", "sce_sys/param.json", "sce_sys/icon0.dds", "sce_sys/pic0.dds", "sce_sys/keystone"};
    for (const auto& entry : entries) if (generated.contains(entry.Path)) throw std::invalid_argument("app0 already contains generated file: " + entry.Path);
    std::array<std::byte, 32> passcode{};
    if (options.Format == PackageFormat::Pkg) passcode = options.Keys->Passcode;
    else passcode.fill(std::byte{'0'});
    const auto keystone = KeystoneBuilder::Build(passcode);
    const auto param = ParamBuilder::Build(options.Parameters);
    const auto icon = IconBuilder::ReadPng(options.App0 / "sce_sys/icon0.png");
    const auto background = IconBuilder::ReadPng(options.App0 / "sce_sys/pic0.png");
    if (icon.Width != 512 || icon.Height != 512) throw std::invalid_argument("icon0.png must be 512x512");
    if (background.Width != 3840 || background.Height != 2160) throw std::invalid_argument("pic0.png must be 3840x2160");
    const auto elf = Ps5ElfBuilder::Build(Files::Read(options.InputElf), options.Imports, options.Elf);
    const auto eboot = EbootPackager::Build(elf, options.Eboot);
    const auto iconDds = IconBuilder::EncodeDds(icon);
    const auto backgroundDds = IconBuilder::EncodeDds(background);
    if (!std::filesystem::create_directory(workspace)) throw std::invalid_argument("Workspace must be new");
    const auto app0 = workspace / "app0";
    std::filesystem::create_directory(app0);
    for (const auto& entry : entries) {
        const auto destination = app0 / Files::Path(entry.Path);
        if (entry.Directory) std::filesystem::create_directories(destination);
        else {
            std::filesystem::create_directories(destination.parent_path());
            if (!std::filesystem::copy_file(entry.Source, destination)) throw std::runtime_error("Cannot copy app0 file");
        }
    }
    Files::Write(workspace / "application.elf", elf);
    Files::Write(app0 / "eboot.bin", eboot);
    Files::Write(app0 / "sce_sys/keystone", keystone);
    Files::Write(app0 / "sce_sys/param.json", std::as_bytes(std::span(param)));
    Files::Write(app0 / "sce_sys/icon0.dds", iconDds);
    Files::Write(app0 / "sce_sys/pic0.dds", backgroundDds);
    if (options.Format == PackageFormat::Ffpfsc) FfpfscPackager::PackDirectory(app0, output, workspace / "package", options.Pfs);
    else {
        PkgOptions pkg;
        pkg.Parameters = options.Parameters;
        pkg.Keys = *options.Keys;
        pkg.Timestamp = options.Pfs.Timestamp;
        PkgPackager::PackDirectory(app0, output, workspace / "package", pkg);
    }
}
}
