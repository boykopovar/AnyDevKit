#include "Commands/ProjectCommand.hpp"
#include "Commands/BuildCommand.hpp"
#include "Project/Json.hpp"
#include "IO/Files.hpp"
#include "ParamBuilder.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
namespace {
void write(const std::filesystem::path& path, const std::string& text) {
    Files::Write(path, std::as_bytes(std::span(text)));
}
std::string read(const std::filesystem::path& path) {
    const auto bytes = Files::Read(path);
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}
void replace(std::string& text, std::string_view token, const std::string& value) {
    std::size_t offset = 0;
    while ((offset = text.find(token, offset)) != std::string::npos) { text.replace(offset, token.size(), value); offset += value.size(); }
}
}
void ProjectCommand::Init(const std::string& name, Arguments& arguments, const std::filesystem::path& sdkRoot) {
    if (name.empty() || name.size() > 64 || !std::ranges::all_of(name, [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }) || (name.front() >= '0' && name.front() <= '9')) throw std::invalid_argument("Project name must be a C++ identifier of at most 64 characters");
    const auto directory = Files::Path(arguments.Optional("directory", name));
    const auto sdk = std::filesystem::canonical(Files::Path(arguments.Optional("sdk", Files::Text(sdkRoot))));
    const auto dependencyRoot = std::filesystem::canonical(Files::Path(arguments.Require("deps")));
    if (!std::filesystem::is_directory(dependencyRoot)) throw std::invalid_argument("Dependency path must be a directory");
    std::uint32_t hash = 2166136261u;
    for (unsigned char c : name) hash = (hash ^ c) * 16777619u;
    auto suffix = std::to_string(hash % 100000);
    suffix.insert(0, 5 - suffix.size(), '0');
    ApplicationParameters parameters;
    parameters.TitleId = arguments.Optional("title-id", "FAKE" + suffix);
    parameters.ContentId = "IV0000-" + parameters.TitleId + "_00-ANYDEVKIT0000001";
    parameters.ConceptId = "10000001";
    parameters.ContentVersion = "01.000.000";
    parameters.MasterVersion = "01.00";
    parameters.DefaultLanguage = "en-US";
    parameters.Titles.emplace("en-US", name);
    parameters.RequiredSystemSoftwareVersion = 0x0200000000000000;
    parameters.SdkVersion = 0x0200000000000000;
    arguments.Finish();
    const auto metadata = ParamBuilder::Build(parameters);
    Files::RequireNew(directory);
    std::map<std::filesystem::path, std::string> files;
    for (const auto& file : {"CMakeLists.txt", "src/Main.cpp", ".gitignore"}) {
        auto text = read(sdk / "share/AnyDevKit/templates" / (std::string(file) + ".in"));
        replace(text, "@NAME@", name);
        files.emplace(file, std::move(text));
    }
    nlohmann::json presets{{"version", 3}, {"cmakeMinimumRequired", {{"major", 3}, {"minor", 22}, {"patch", 1}}}};
    presets["configurePresets"] = nlohmann::json::array();
    presets["buildPresets"] = nlohmann::json::array();
    for (const auto& target : {"windows", "linux", "ps5"}) {
        const auto dependencies = Files::Text(dependencyRoot / target);
        const nlohmann::json cache{{"CMAKE_BUILD_TYPE", "Release"}, {"CMAKE_TOOLCHAIN_FILE", Files::Text(sdk / "lib/cmake/AnyDevKit/toolchains" / (std::string(target) + ".cmake"))}, {"AnyDevKit_DIR", Files::Text(sdk / "lib/cmake/AnyDevKit")}, {"ADK_DEPENDENCY_ROOT", dependencies}, {"ADK_SYSROOT", dependencies + "/sysroot"}};
        presets["configurePresets"].push_back({{"name", target}, {"generator", "Ninja"}, {"binaryDir", std::string("${sourceDir}/build/") + target}, {"cacheVariables", cache}});
        presets["buildPresets"].push_back({{"name", target}, {"configurePreset", target}});
    }
    nlohmann::json options{{"input", "build/ps5/" + name + ".elf"}, {"app0", "app0"}, {"param", "app0/sce_sys/param.json"}, {"output", "dist/ps5/" + name + ".ffpfsc"}, {"workspace", "build/package"}, {"format", "ffpfsc"}, {"module-name", name}, {"file-name", name + ".elf"}, {"elf-sdk-version", "0x02000009"}, {"companion-sdk-version", "0x08050001"}, {"authority", "0x3100000000000002"}, {"application-version", "0"}, {"self-firmware-version", "0"}, {"timestamp", "0"}, {"select-import", nlohmann::json::array()}};
    const nlohmann::json config{{"schemaVersion", 1}, {"name", name}, {"targets", {{"ps5", options}}}};
    files.emplace("CMakePresets.json", presets.dump(2) + "\n");
    files.emplace("adk.json", config.dump(2) + "\n");
    files.emplace("app0/sce_sys/param.json", metadata);
    std::filesystem::create_directory(directory);
    std::filesystem::create_directories(directory / "shaders");
    for (const auto& [path, text] : files) {
        std::filesystem::create_directories((directory / path).parent_path());
        write(directory / path, text);
    }
}
void ProjectCommand::Package(Arguments& arguments) {
    const auto project = std::filesystem::canonical(Files::Path(arguments.Optional("project", ".")));
    const auto target = arguments.Require("target");
    if (target != "ps5") throw std::invalid_argument("package supports only the ps5 target; desktop output is produced by CMake");
    const auto config = Json::Read(project / "adk.json");
    Json::Fields(config, {"schemaVersion", "name", "targets"});
    if (!config.at("schemaVersion").is_number_unsigned() || config.at("schemaVersion") != 1) throw std::invalid_argument("Unsupported project schemaVersion");
    if (config.at("name").get<std::string>().empty()) throw std::invalid_argument("Empty project name");
    Json::Fields(config.at("targets"), {"ps5"});
    const auto& options = config.at("targets").at(target);
    const std::set<std::string> paths{"input", "app0", "param", "output", "workspace", "passcode-moduli", "mount-modulus"};
    std::set<std::string> allowed = paths;
    for (const auto& name : {"format", "module-name", "file-name", "elf-sdk-version", "companion-sdk-version", "select-import", "authority", "application-version", "self-firmware-version", "block-size", "timestamp", "case-insensitive", "passcode", "image-seed"}) allowed.insert(name);
    Json::Fields(options, allowed);
    std::map<std::string, std::vector<std::string>> defaults;
    for (const auto& [name, value] : options.items()) {
        if (name == "case-insensitive") {
            if (!value.is_boolean()) throw std::invalid_argument("case-insensitive must be boolean");
            defaults[name] = {value.get<bool>() ? "true" : "false"};
        } else if (name == "select-import") {
            if (!value.is_array()) throw std::invalid_argument("select-import must be an array");
            for (const auto& item : value) defaults[name].push_back(item.get<std::string>());
        } else {
            auto text = value.is_string() ? value.get<std::string>() : value.is_number_unsigned() ? value.dump() : throw std::invalid_argument("Expected string or unsigned integer: " + name);
            if (paths.contains(name)) text = Files::Text(project / Files::Path(text));
            defaults[name] = {text};
        }
    }
    arguments.MergeDefaults(defaults);
    const auto output = arguments.Require("output");
    const auto workspace = arguments.Require("workspace");
    arguments.MergeDefaults({{"output", {output}}, {"workspace", {workspace}}});
    for (const auto& path : {Files::Path(output), Files::Path(workspace)}) if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    BuildCommand::Run("build", arguments);
}
void ProjectCommand::Dependencies(std::string_view command, Arguments& arguments) {
    if (command == "setup") {
        const auto target = arguments.Require("target");
        if (target != "windows" && target != "linux" && target != "ps5") throw std::invalid_argument("Unknown setup target: " + target);
    } else if (command != "update") throw std::invalid_argument("Unknown dependency command");
    static_cast<void>(arguments.Optional("project", "."));
    arguments.Finish();
    throw std::runtime_error(std::string(command) + " is not implemented; install release packages manually and configure CMakePresets.json");
}
}
