#ifndef ANYDEVKIT_PUBLICSDK_OPTIONS_HPP
#define ANYDEVKIT_PUBLICSDK_OPTIONS_HPP
#include "CommandLine/Arguments.hpp"
#include "EbootPackager.hpp"
#include "PfsBuilder.hpp"
#include "PkgPackager.hpp"
#include "Ps5ElfBuilder.hpp"
namespace AnyDevKit::PublicSdk {
class Options final {
public:
    static ApplicationParameters Parameters(Arguments& arguments);
    static Pkg::PackageKeys Keys(Arguments& arguments);
    static PfsOptions Pfs(Arguments& arguments);
    static EbootPackageOptions Eboot(Arguments& arguments);
    static Ps5ElfOptions Elf(Arguments& arguments);
    static ImportCatalog Imports(Arguments& arguments, const std::filesystem::path& input);
    static std::int64_t Timestamp(Arguments& arguments);
};
}
#endif
