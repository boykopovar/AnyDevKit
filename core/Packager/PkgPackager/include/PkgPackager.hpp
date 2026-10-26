#ifndef ANYDEVKIT_PKG_PACKAGER_HPP
#define ANYDEVKIT_PKG_PACKAGER_HPP

#include "Domain/PackageKeys.hpp"
#include "ParamBuilder.hpp"

namespace AnyDevKit {

struct PkgOptions {
    ApplicationParameters Parameters;
    Pkg::PackageKeys Keys;
    std::int64_t Timestamp = 0;
};

class PkgPackager final {
public:
    static void PackDirectory(const std::filesystem::path& input, const std::filesystem::path& output, const std::filesystem::path& workspace, const PkgOptions& options);
};

}

#endif
