#include "FfpfscPackager.hpp"

#include <stdexcept>

namespace AnyDevKit {

void FfpfscPackager::PackDirectory(const std::filesystem::path& input, const std::filesystem::path& output, const std::filesystem::path& workspace, const PfsOptions& options) {
    const auto sourceRoot = std::filesystem::canonical(input);
    const auto destination = std::filesystem::weakly_canonical(output);
    const auto workRoot = std::filesystem::weakly_canonical(workspace);
    const auto isWithin = [](const auto& parent, const auto& child) {
        const auto relative = child.lexically_relative(parent);
        return !relative.empty() && *relative.begin() != "..";
    };
    if (isWithin(sourceRoot, destination) || isWithin(sourceRoot, workRoot) || isWithin(workRoot, destination)) throw std::invalid_argument("FFPFSC input, workspace and output must be separate");
    if (std::filesystem::exists(std::filesystem::symlink_status(output))) throw std::invalid_argument("FFPFSC output already exists");
    const auto entries = PfsBuilder::ScanDirectory(sourceRoot);
    if (!std::filesystem::create_directory(workRoot)) throw std::invalid_argument("FFPFSC workspace must be a new directory");
    const auto inner = workRoot / "pfs_image.dat";
    PfsBuilder::Build(entries, inner, options);
    PfsBuilder::Build({{"pfs_image.dat", inner, false, true}}, output, options);
    std::filesystem::remove(inner);
    std::filesystem::remove(workRoot);
}

}
