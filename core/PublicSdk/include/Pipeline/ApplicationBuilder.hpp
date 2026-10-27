#ifndef ANYDEVKIT_PUBLICSDK_APPLICATIONBUILDER_HPP
#define ANYDEVKIT_PUBLICSDK_APPLICATIONBUILDER_HPP
#include "EbootPackager.hpp"
#include "PfsBuilder.hpp"
#include "PkgPackager.hpp"
#include "Ps5ElfBuilder.hpp"
#include <optional>
namespace AnyDevKit::PublicSdk {
enum class PackageFormat { Ffpfsc, Pkg };
struct ApplicationBuildOptions {
    std::filesystem::path InputElf;
    std::filesystem::path App0;
    std::filesystem::path Output;
    std::filesystem::path Workspace;
    PackageFormat Format = PackageFormat::Ffpfsc;
    ApplicationParameters Parameters;
    ImportCatalog Imports;
    Ps5ElfOptions Elf;
    EbootPackageOptions Eboot;
    PfsOptions Pfs;
    std::optional<Pkg::PackageKeys> Keys;
};
class ApplicationBuilder final {
public:
    static void Build(const ApplicationBuildOptions& options);
};
}
#endif
