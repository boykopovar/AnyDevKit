#include "CommandLine/Options.hpp"
#include "ShaderCompiler.hpp"
#include "Shader/VertexAttribute.hpp"
#include "IO/Files.hpp"
#include "Project/Metadata.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <limits>
#include <stdexcept>

namespace AnyDevKit::PublicSdk {

PsbcCompileOptions Options::Shader(Arguments& arguments, PsbcStage stage) {
    auto options = ShaderCompiler::CreateOptions(stage, arguments.Optional("target", "ps5"));
    const auto omit = arguments.Optional("omit-implicit-primitive-id", options.omit_implicit_primitive_id ? "true" : "false");
    if (omit != "true" && omit != "false") throw std::invalid_argument("Invalid --omit-implicit-primitive-id value");
    options.omit_implicit_primitive_id = omit == "true";

    for (const auto& value : arguments.Many("vertex-attribute")) {
        if (options.vertex_attribute_count >= PSBC_MAX_VERTEX_ATTRIBUTES) throw std::out_of_range("Vertex attribute table exceeds capacity");
        options.vertex_attributes[options.vertex_attribute_count++] = VertexAttribute::Parse(value);
    }

    for (const auto& value : arguments.Many("descriptor")) {
        std::array<std::string, 6> fields;
        std::size_t begin = 0;
        for (std::size_t index = 0; index < fields.size(); ++index) {
            const auto end = value.find(',', begin);
            if ((end == std::string::npos) != (index == fields.size() - 1)) throw std::invalid_argument("Expected --descriptor set,binding,type,count,offset,stride");
            fields[index] = value.substr(begin, end == std::string::npos ? end : end - begin);
            if (fields[index].empty()) throw std::invalid_argument("Empty descriptor field");
            begin = end == std::string::npos ? value.size() : end + 1;
        }

        const std::map<std::string, PsbcDescriptorType> types{
            {"uniform-buffer", PSBC_DESCRIPTOR_UNIFORM_BUFFER}, {"storage-buffer", PSBC_DESCRIPTOR_STORAGE_BUFFER},
            {"uniform-texel-buffer", PSBC_DESCRIPTOR_UNIFORM_TEXEL_BUFFER}, {"storage-texel-buffer", PSBC_DESCRIPTOR_STORAGE_TEXEL_BUFFER},
            {"combined-image-sampler", PSBC_DESCRIPTOR_COMBINED_IMAGE_SAMPLER}, {"sampled-image", PSBC_DESCRIPTOR_SAMPLED_IMAGE},
            {"storage-image", PSBC_DESCRIPTOR_STORAGE_IMAGE}, {"sampler", PSBC_DESCRIPTOR_SAMPLER}, {"input-attachment", PSBC_DESCRIPTOR_INPUT_ATTACHMENT}
        };
        const auto type = types.find(fields[2]);
        if (type == types.end()) throw std::invalid_argument("Unknown descriptor type: " + fields[2]);
        const auto set = Arguments::Number32(fields[0]);
        const auto binding = Arguments::Number32(fields[1]);
        if (set >= PSBC_MAX_DESCRIPTOR_SETS || binding > 255 || options.descriptor_binding_count == PSBC_MAX_DESCRIPTOR_BINDINGS) throw std::invalid_argument("Descriptor exceeds layout capacity");
        const auto count = Arguments::Number32(fields[3]);
        const auto offset = Arguments::Number32(fields[4]);
        const auto stride = Arguments::Number32(fields[5]);
        if (!count || !stride || offset % 4 || stride % 4 || count > (std::numeric_limits<std::uint32_t>::max() - offset) / stride) throw std::invalid_argument("Invalid descriptor extent");

        for (std::uint32_t index = 0; index < options.descriptor_binding_count; ++index) {
            const auto& previous = options.descriptor_bindings[index];
            if (previous.set == set && (previous.binding == binding || (offset < previous.offset + previous.array_size * previous.stride && previous.offset < offset + count * stride))) throw std::invalid_argument("Duplicate or overlapping descriptor");
        }

        options.descriptor_bindings[options.descriptor_binding_count++] = {static_cast<std::uint8_t>(set), static_cast<std::uint8_t>(binding), type->second, count, offset, stride};
    }

    return options;
}

ApplicationParameters Options::Parameters(Arguments& arguments) {
    const auto param = arguments.Optional("param");
    if (!param.empty()) arguments.MergeDefaults(Metadata::Read(Files::Path(param)));
    ApplicationParameters result;
    result.TitleId = arguments.Require("title-id");
    result.ContentId = arguments.Require("content-id");
    result.ConceptId = arguments.Require("concept-id");
    result.ContentVersion = arguments.Require("content-version");
    result.MasterVersion = arguments.Require("master-version");
    result.DefaultLanguage = arguments.Require("default-language");

    for (const auto& title : arguments.Many("title")) {
        const auto split = title.find('=');
        if (split == std::string::npos || !result.Titles.emplace(title.substr(0, split), title.substr(split + 1)).second) throw std::invalid_argument("Expected unique --title language=text");
    }

    result.RequiredSystemSoftwareVersion = Arguments::Number(arguments.Require("firmware-version"));
    result.SdkVersion = Arguments::Number(arguments.Require("sdk-version"));
    result.ApplicationCategoryType = Arguments::Number32(arguments.Optional("category", "0"));
    result.Attribute = Arguments::Number32(arguments.Optional("attribute", "0"));
    result.Attribute2 = Arguments::Number32(arguments.Optional("attribute2", "0"));
    result.Attribute3 = Arguments::Number32(arguments.Optional("attribute3", "0"));
    const auto memory = arguments.Optional("flexible-memory");
    if (!memory.empty()) result.FlexibleMemorySize = Arguments::Number(memory);
    static_cast<void>(ParamBuilder::Build(result));

    return result;
}

Pkg::PackageKeys Options::Keys(Arguments& arguments) {
    Pkg::PackageKeys keys;
    const auto moduli = Files::Read(Files::Path(arguments.Require("passcode-moduli")));
    const auto mount = Files::Read(Files::Path(arguments.Require("mount-modulus")));
    if (moduli.size() != 7 * 384 || mount.size() != 384) throw std::invalid_argument("Expected seven 384-byte passcode moduli and one 384-byte mount modulus");

    for (std::size_t i = 0; i < keys.PasscodeModuli.size(); ++i) std::copy_n(moduli.begin() + static_cast<std::ptrdiff_t>(i * 384), 384, keys.PasscodeModuli[i].begin());
    std::copy(mount.begin(), mount.end(), keys.MountModulus.begin());
    const auto passcode = arguments.Require("passcode");
    const auto seed = arguments.Require("image-seed");
    const auto hex = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<unsigned>(c - 'A' + 10);
        throw std::invalid_argument("Invalid hexadecimal key character");
    };
    if (passcode.size() != 32 || seed.size() != 32) throw std::invalid_argument("Passcode and image seed must each contain 32 hexadecimal characters");

    for (std::size_t i = 0; i < passcode.size(); ++i) {
        static_cast<void>(hex(passcode[i]));
        keys.Passcode[i] = static_cast<std::byte>(passcode[i]);
    }

    for (std::size_t i = 0; i < keys.ImageSeed.size(); ++i) keys.ImageSeed[i] = static_cast<std::byte>((hex(seed[2 * i]) << 4) | hex(seed[2 * i + 1]));
    return keys;
}

std::int64_t Options::Timestamp(Arguments& arguments) {
    const auto value = Arguments::Number(arguments.Optional("timestamp", "0"));
    if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) throw std::invalid_argument("Timestamp exceeds signed 64 bits");
    return static_cast<std::int64_t>(value);
}

PfsOptions Options::Pfs(Arguments& arguments) {
    PfsOptions result;
    result.BlockSize = Arguments::Number32(arguments.Optional("block-size", "65536"));
    result.Timestamp = Timestamp(arguments);
    result.CaseInsensitive = arguments.Flag("case-insensitive");

    return result;
}

EbootPackageOptions Options::Eboot(Arguments& arguments) {
    EbootPackageOptions result;
    result.ProgramAuthorityId = Arguments::Number(arguments.Optional("authority", "0x3100000000000002"));
    result.ApplicationVersion = Arguments::Number(arguments.Optional("application-version", "0"));
    result.FirmwareVersion = Arguments::Number(arguments.Optional("self-firmware-version", "0"));

    return result;
}

Ps5ElfOptions Options::Elf(Arguments& arguments) {
    Ps5ElfOptions result;
    result.ModuleName = arguments.Require("module-name");
    result.FileName = arguments.Require("file-name");
    result.SdkVersion = Arguments::Number32(arguments.Require("elf-sdk-version"));
    result.CompanionSdkVersion = Arguments::Number32(arguments.Require("companion-sdk-version"));
    const auto heapSize = arguments.Optional("heap-size");
    if (!heapSize.empty()) result.HeapSize = Arguments::Number(heapSize);
    return result;
}

ImportCatalog Options::Imports(Arguments& arguments, const std::filesystem::path& input) {
    ImportCatalog catalog;
    for (const auto& module : ImportCatalog::Dependencies(Files::Read(input))) catalog.AddModule(module);
    for (const auto& selection : arguments.Many("select-import")) {
        const auto first = selection.find(',');
        const auto second = first == std::string::npos ? first : selection.find(',', first + 1);
        if (first == std::string::npos || second == std::string::npos || selection.find(',', second + 1) != std::string::npos) throw std::invalid_argument("Expected --select-import symbol,module,library");
        catalog.Select(selection.substr(0, first), selection.substr(first + 1, second - first - 1), selection.substr(second + 1));
    }

    return catalog;
}

}

