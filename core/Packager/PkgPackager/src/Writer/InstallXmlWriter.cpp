#include "Writer/InstallXmlWriter.hpp"
#include "Writer/ImageWriter.hpp"

#include <algorithm>
#include <stdexcept>

namespace AnyDevKit::Pkg {
namespace {

std::string Escape(std::string_view value) {
    std::string result;
    for (const auto c : value) {
        switch (c) {
        case '&': result += "&amp;"; break;
        case '<': result += "&lt;"; break;
        case '>': result += "&gt;"; break;
        case '\"': result += "&quot;"; break;
        case '\'': result += "&apos;"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) throw std::invalid_argument("Invalid XML text character");
            result += c;
        }
    }
    return result;
}

std::string Hex(std::uint64_t value, std::size_t digits = 16) {
    std::string result(digits + 2, '0');
    result[1] = 'x';
    for (std::size_t i = 0; i < digits; ++i) result[digits + 1 - i] = "0123456789abcdef"[(value >> (i * 4)) & 15];
    return result;
}

std::string Digest(std::span<const std::byte> data) {
    std::string result;
    for (const auto b : data) {
        if (!result.empty()) result += ' ';
        result += Hex(std::to_integer<unsigned>(b), 2);
    }
    return result;
}

}

std::string InstallXmlWriter::Build(const ApplicationParameters& parameters, const PackageKeys& keys, const InnerImageInfo& inner, const OuterImageInfo& outer, const CntImage& container) {
    if (container.Data.size() < 0x2000) throw std::invalid_argument("Truncated CNT for install XML");
    const auto cnt = std::span(container.Data);
    const auto entry = [&](std::uint32_t id) -> const CntEntryInfo& {
        const auto found = std::find_if(container.Entries.begin(), container.Entries.end(), [&](const auto& item) { return item.Id == id; });
        if (found == container.Entries.end()) throw std::invalid_argument("Missing CNT entry for install XML");
        if (std::uint64_t{found->Offset} + found->Size > cnt.size()) throw std::invalid_argument("CNT entry outside container");
        return *found;
    };
    const auto general = entry(0x80);
    if (general.Size < 0x1e0) throw std::invalid_argument("Truncated CNT general digests");
    const auto containerOffset = Pfs::ImageWriter::Add(0x10000, outer.Size);
    const auto mountSize = Pfs::ImageWriter::Add(containerOffset, cnt.size());
    std::string result = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<package-configuration version=\"1.0\" type=\"package-info\">\n";
    const auto field = [&](std::string_view name, std::string_view value) { result += "<" + std::string(name) + ">" + Escape(value) + "</" + std::string(name) + ">\n"; };
    result += "<config version=\"" + Escape(parameters.ContentVersion) + "\" metadata=\"0\" primary=\"yes\">\n";
    field("content-id", parameters.ContentId);
    field("primary-id", parameters.ContentId);
    auto version = parameters.ContentVersion;
    std::erase(version, '.');
    field("longname", parameters.ContentId + "-C" + version + "-M0000000000000000-GD");
    auto systemVersion = Hex(parameters.RequiredSystemSoftwareVersion).substr(2);
    field("required-system-version", systemVersion.substr(0, 2) + "." + systemVersion.substr(2, 3) + "." + systemVersion.substr(5, 3) + "." + systemVersion.substr(8));
    field("drm-type", "none");
    field("content-type", "PS5GD");
    field("application-type", "free");
    field("num-of-images", "1");
    field("package-size", std::to_string(mountSize));
    field("version-date", "0x20200722");
    field("version-hash", "0x01fe52e9");
    result += "</config>\n<digests version=\"1.2\" major-param-version=\"0\">\n";
    for (const auto& [name, index] : {std::pair{"content-digest", 0}, {"game-digest", 1}, {"header-digest", 2}, {"system-digest", 3}, {"param-digest", 5}}) field(name, Digest(cnt.subspan(general.Offset + 0x20 + index * 32, 32)));
    field("package-digest", Digest(cnt.subspan(0xfe0, 32)));
    result += "</digests>\n<params>\n";
    field("applicationDrmType", "free");
    field("contentId", parameters.ContentId);
    field("contentVersion", parameters.ContentVersion);
    field("masterVersion", parameters.MasterVersion);
    field("requiredSystemSoftwareVersion", Hex(parameters.RequiredSystemSoftwareVersion));
    field("sdkVersion", Hex(parameters.SdkVersion));
    field("titleName", parameters.Titles.at(parameters.DefaultLanguage));
    result += "</params>\n<container nth-of-image=\"1\">\n";
    field("container-size", Hex(cnt.size()));
    field("mandatory-size", Hex(entry(0x40a).Offset));
    field("body-offset", Hex(0x2000));
    field("body-size", Hex(cnt.size() - 0x2000));
    field("body-digest", Digest(cnt.subspan(0x160, 32)));
    field("promote-size", Hex(cnt.size(), 8));
    result += "</container>\n<mount-image nth-of-image=\"1\" nested-image=\"yes\">\n";
    for (const auto name : {"pfs-offset-align", "pfs-size-align", "pfs-image-offset"}) field(name, Hex(0x10000));
    field("pfs-image-size", Hex(outer.Size));
    field("fixed-info-size", Hex(0x10000, 8));
    field("pfs-image-seed", Digest(keys.ImageSeed));
    field("sblock-digest", Digest(outer.SuperblockDigest));
    field("fixed-info-digest", Digest(cnt.subspan(0x460, 32)));
    field("mount-image-offset", Hex(0));
    field("mount-image-size", Hex(mountSize));
    field("container-offset", Hex(containerOffset));
    field("supplemental-offset", Hex(cnt.size()));
    result += "</mount-image>\n";
    const auto count = std::count_if(container.Entries.begin(), container.Entries.end(), [](const auto& item) { return !item.Name.empty(); });
    result += "<entries nth-of-image=\"1\" num=\"" + std::to_string(count) + "\">\n";
    for (const auto& item : container.Entries) if (!item.Name.empty()) result += "<entry offset=\"" + Hex(item.Offset, 8) + "\" size=\"" + Hex(item.Size, 8) + "\" name=\"" + Escape(item.Name) + "\"/>\n";
    result += "</entries>\n<chunkinfo size=\"416\" nested=\"true\" sdk=\"0x00850000\" disps=\"0x0011\">\n";
    field("contentid", parameters.ContentId);
    const auto total = std::to_string(containerOffset);
    result += "<languages default=\"1\">0xffffffffffffffff</languages>\n<scenarios num=\"1\" default=\"0\" groups=\"0\"><scenario id=\"0\" type=\"33\" name=\"Scenario #0\">\n";
    for (const auto name : {"overall", "default"}) result += "<" + std::string(name) + " initials=\"1\" num=\"1\" init-size=\"" + total + "\" total=\"" + total + "\">0</" + name + ">\n";
    result += "</scenario></scenarios>\n<chunks num=\"1\" default=\"0xffffffffffffffff\"><chunk id=\"0\" flag=\"0x80\" locus=\"0x03\" language=\"0xffffffffffffffff\" disps=\"0x0011\" num=\"2\" size=\"" + total + "\" name=\"Chunk #0\">0 1</chunk></chunks>\n<outers num=\"2\" overlapped=\"0\" language-overlapped=\"0\">\n";
    result += "<outer id=\"0\" image=\"0\" offset=\"" + Hex(0) + "\" size=\"" + Hex(inner.Size) + "\" chunks=\"1\"/>\n";
    result += "<outer id=\"1\" image=\"0\" offset=\"" + Hex(inner.Size) + "\" size=\"" + Hex(containerOffset - inner.Size) + "\" chunks=\"1\"/>\n</outers></chunkinfo>\n</package-configuration>\n";
    return result;
}

}
