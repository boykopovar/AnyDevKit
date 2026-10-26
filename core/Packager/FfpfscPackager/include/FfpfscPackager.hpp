#ifndef ANYDEVKIT_PACKAGER_FFPFSCPACKAGER_HPP
#define ANYDEVKIT_PACKAGER_FFPFSCPACKAGER_HPP

#include "PfsBuilder.hpp"

namespace AnyDevKit {

class FfpfscPackager final {
public:
    static void PackDirectory(const std::filesystem::path& input, const std::filesystem::path& output, const std::filesystem::path& workspace, const PfsOptions& options = {});
};

}

#endif
