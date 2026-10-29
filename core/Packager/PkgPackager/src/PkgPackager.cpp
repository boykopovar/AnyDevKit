#include "PkgPackager.hpp"
#include "Builder/CntBuilder.hpp"
#include "Builder/InnerImageBuilder.hpp"
#include "Builder/InstallMetadataBuilder.hpp"
#include "Builder/PlayGoBuilder.hpp"
#include "Writer/FihWriter.hpp"
#include "Writer/NapsWriter.hpp"
#include "Writer/InstallArchiveWriter.hpp"
#include "Writer/InstallXmlWriter.hpp"
#include "Writer/ImageWriter.hpp"
#include "PfsBuilder.hpp"
#include "Crc32.hpp"
#include "Sha256.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <stdexcept>

namespace AnyDevKit {
namespace {

std::vector<std::byte> Bytes(std::string_view text) {
    const auto bytes = std::as_bytes(std::span(text.data(), text.size()));
    return {bytes.begin(), bytes.end()};
}

std::vector<std::byte> Read(const std::filesystem::path& path) {
    const auto size = std::filesystem::file_size(path);
    if (size > 0xffff0000) throw std::length_error("PKG metadata file exceeds format range");
    std::vector<std::byte> result(static_cast<std::size_t>(size));
    std::ifstream source;
    source.exceptions(std::ios::badbit | std::ios::failbit);
    source.open(path, std::ios::binary);
    source.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(result.size()));
    if (source.rdbuf()->sgetc() != std::char_traits<char>::eof()) throw std::runtime_error("PKG metadata source changed while reading");
    return result;
}

void Write(const std::filesystem::path& path, std::span<const std::byte> data) {
    Pfs::ImageWriter writer(path);
    writer.Write(0, data);
    writer.Close();
}

std::vector<std::byte> ChunkChecks(const std::filesystem::path& path, std::uint64_t size) {
    if (!size || size % 0x10000 || size / 0x10000 > 0x3fffffff) throw std::length_error("Invalid mount image CRC geometry");
    std::vector<std::byte> result(static_cast<std::size_t>(size / 0x10000 * 4));
    std::array<std::byte, 0x10000> block{};
    std::ifstream stream;
    stream.exceptions(std::ios::badbit | std::ios::failbit);
    stream.open(path, std::ios::binary);
    for (std::size_t i = 0; i < result.size(); i += 4) {
        stream.read(reinterpret_cast<char*>(block.data()), block.size());
        Pfs::ImageWriter::Put(result, i, Crypto::Crc32::Castagnoli(block), 4);
    }
    return result;
}

}

void PkgPackager::PackDirectory(const std::filesystem::path& input, const std::filesystem::path& output, const std::filesystem::path& workspace, const PkgOptions& options) {
    using namespace Pkg;
    using Pfs::ImageWriter;
    const auto generatedParam = ParamBuilder::Build(options.Parameters);
    const auto parameterData = Bytes(options.ParameterJson ? *options.ParameterJson : generatedParam);
    if (options.Timestamp < 0) throw std::invalid_argument("Negative PKG timestamp");
    const auto root = std::filesystem::canonical(input);
    const auto destination = std::filesystem::weakly_canonical(output);
    const auto work = std::filesystem::weakly_canonical(workspace);
    const auto within = [](const auto& parent, const auto& child) {
        const auto relative = child.lexically_relative(parent);
        return !relative.empty() && *relative.begin() != "..";
    };
    if (within(root, destination) || within(root, work) || within(work, root) || within(work, destination)) throw std::invalid_argument("PKG input, workspace and output must be separate");
    if (std::filesystem::exists(std::filesystem::symlink_status(destination))) throw std::invalid_argument("PKG output already exists");
    const auto files = PfsBuilder::ScanDirectory(root);
    const auto executable = std::find_if(files.begin(), files.end(), [](const auto& file) { return file.Path == "eboot.bin" && !file.Directory; });
    if (executable == files.end()) throw std::invalid_argument("PKG application requires eboot.bin");
    std::array<unsigned char, 4> magic{};
    std::ifstream executableStream(executable->Source, std::ios::binary);
    executableStream.read(reinterpret_cast<char*>(magic.data()), magic.size());
    if (!executableStream || magic != std::array<unsigned char, 4>{0x4f, 0x15, 0x3d, 0x1d}) throw std::invalid_argument("PKG eboot.bin must be a SELF executable");
    executableStream.close();
    const std::map<std::string, std::uint32_t> systemEntries{{"license.dat", 0x400}, {"license.info", 0x401}, {"nptitle.dat", 0x402}, {"npbind.dat", 0x403}, {"pronunciation.xml", 0x1006}, {"pronunciation.sig", 0x100d}, {"icon0.png", 0x1200}, {"pic0.png", 0x1220}, {"pic1.png", 0x1240}, {"icon0.dds", 0x1280}, {"pic0.dds", 0x12a0}, {"pic1.dds", 0x12c0}};
    std::vector<CntEntry> entries{{0x2000, "param.json", parameterData}};
    std::vector<std::string> excluded;
    for (const auto& file : files) {
        if (file.Directory || !file.Path.starts_with("sce_sys/")) continue;
        const auto name = file.Path.substr(8);
        if (name == "param.json") {
            if (Read(file.Source) != parameterData) throw std::invalid_argument("Source param.json differs from supplied application parameters");
            excluded.push_back(file.Path);
        } else if (name == "imagedigs.dat" || name == "playgo-chunk.dat" || name == "playgo-hash-table.dat" || name == "playgo-ficm.dat") {
            throw std::invalid_argument("Remove generated package metadata from application input: " + file.Path);
        } else if (const auto found = systemEntries.find(name); found != systemEntries.end()) {
            entries.push_back({found->second, name, Read(file.Source)});
            excluded.push_back(file.Path);
        }
    }
    const auto keyData = KeyBuilder::Build(options.Parameters.ContentId, options.Keys);
    if (!std::filesystem::create_directory(work)) throw std::invalid_argument("PKG workspace must be a new directory");
    const auto innerPath = work / "inner.dat";
    const auto napsPath = work / "naps.dat";
    const auto outerPath = work / "outer.dat";
    const auto mountPath = work / "mount.dat";
    const auto inner = InnerImageBuilder::Build(root, innerPath, options.Timestamp, excluded);
    const auto naps = NapsWriter::Build(inner);
    Write(napsPath, naps);
    const auto outer = OuterImageBuilder::Build(innerPath, napsPath, inner.Size, keyData.ImageSecret, options.Keys.ImageSeed, options.Timestamp, outerPath);
    std::uint32_t version = 0;
    for (const auto c : options.Parameters.ContentVersion) if (c != '.') version = (version << 4) | static_cast<unsigned>(c - '0');
    const auto fih = FihWriter::Build(inner, outer, naps, version);
    const auto playgo = PlayGoBuilder::Build(options.Parameters.ContentId, inner, outer);
    entries.push_back({0x40a, "imagedigs.dat", outer.BlockDigests});
    entries.push_back({0x1001, "playgo-chunk.dat", playgo.Chunks});
    entries.push_back({0x2010, "playgo-hash-table.dat", playgo.Hashes});
    entries.push_back({0x2011, "playgo-ficm.dat", playgo.FileChunks});
    const auto container = CntBuilder::Build(options.Parameters.ContentId, options.Keys, keyData, outer, fih, entries);
    const auto containerOffset = ImageWriter::Add(fih.size(), outer.Size);
    const auto mountSize = ImageWriter::Add(containerOffset, container.Data.size());
    {
        ImageWriter writer(mountPath);
        writer.Write(0, fih);
        writer.Copy(fih.size(), outerPath, outer.Size);
        writer.Write(containerOffset, container.Data);
        writer.Close();
    }
    const auto metadata = InstallMetadataBuilder::Build(innerPath, inner, Crypto::Sha256::Hmac(keyData.ImageSecret, options.Keys.ImageSeed), options.Keys.ImageSeed);
    std::vector<InstallArchiveEntry> supplemental{{"common/etc/naps_meta_18.dat", metadata.Metrics}};
    for (const auto id : {300, 301, 302, 308}) supplemental.push_back({"common/etc/naps_meta_" + std::to_string(id) + ".dat", metadata.Descriptor});
    supplemental.push_back({"common/etc/pfsimage.xml", Bytes(InstallXmlWriter::Build(options.Parameters, options.Keys, inner, outer, container))});
    supplemental.push_back({"common/etc/playgo-chunk.dat", playgo.Chunks});
    supplemental.push_back({"config/" + options.Parameters.ContentId + "/playgo-chunk.crc", ChunkChecks(mountPath, mountSize)});
    const auto archive = InstallArchiveWriter::Build(supplemental);
    try {
        ImageWriter writer(destination);
        writer.Copy(0, mountPath, mountSize);
        writer.Write(mountSize, archive);
        writer.Close();
    } catch (...) {
        std::filesystem::remove(destination);
        throw;
    }
    for (const auto& path : {innerPath, napsPath, outerPath, mountPath}) if (!std::filesystem::remove(path)) throw std::runtime_error("Cannot remove PKG workspace file");
    if (!std::filesystem::remove(work)) throw std::runtime_error("Cannot remove PKG workspace");
}

}
