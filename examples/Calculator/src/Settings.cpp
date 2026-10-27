#include "Settings.hpp"
#include "CalculatorError.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libSceSaveData.native/libSceSaveData.native.hpp"
#include "prx/libSceSysmodule/libSceSysmodule.hpp"
#include "prx/libc/include/Api.hpp"

#include <cstdint>
#include <algorithm>
#include <span>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

void validate(const Settings::Data& values) {
    const auto& vibration = values.Vibration;
    if (vibration[0] < 0 || vibration[0] > 200 || vibration[0] % 5 != 0 || vibration[1] < 0 || vibration[1] > 255 || vibration[2] < 0 || vibration[2] > 255) throw CalculatorError{"Invalid vibration settings"};
    if (values.History.Count > values.History.Entries.size()) throw CalculatorError{"Invalid saved history count"};
    constexpr std::string_view allowed = "0123456789 .+-*/=E";
    for (std::size_t i = 0; i < values.History.Count; ++i) {
        const auto& entry = values.History.Entries[i];
        const auto end = std::find(entry.begin(), entry.end(), 0);
        if (end == entry.begin() || end == entry.end()) throw CalculatorError{"Invalid saved history text"};
        for (auto it = entry.begin(); it != end; ++it) if (allowed.find(*it) == std::string_view::npos) throw CalculatorError{"Invalid saved history character"};
    }
}

void readExact(int file, std::span<std::uint8_t> bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto count = sceKernelRead(file, bytes.data() + offset, bytes.size() - offset);
        if (count < 0) throw CalculatorError{"Read calculator settings", static_cast<int>(count)};
        if (count == 0) throw CalculatorError{"Truncated calculator settings"};
        offset += static_cast<std::size_t>(count);
    }
}

Settings::Data readFile(const char* path) {
    const int file = check(sceKernelOpen(path, SCE_KERNEL_O_RDONLY, 0), "Open calculator settings");
    Settings::Data values{};
    try {
        std::array<std::uint8_t, 8> header{};
        readExact(file, header);
        if (header[0] != 'A' || header[1] != 'D' || header[2] != 'K' || header[3] != 'V' || header[4] != 3) throw CalculatorError{"Unsupported calculator settings format"};
        values.Vibration = {header[5], header[6], header[7]};
        std::uint8_t extra = 0;
        const auto count = sceKernelRead(file, &extra, 1);
        if (count < 0) throw CalculatorError{"Read calculator settings", static_cast<int>(count)};
        if (count != 0) throw CalculatorError{"Unexpected calculator settings data"};
        validate(values);
    } catch (...) {
        check(sceKernelClose(file), "Close calculator settings");
        throw;
    }
    check(sceKernelClose(file), "Close calculator settings");
    return values;
}

void readHistory(const char* path, Settings::Data& values) {
    const int file = check(sceKernelOpen(path, SCE_KERNEL_O_RDONLY, 0), "Open calculator history");
    try {
        std::array<std::uint8_t, 166> bytes{};
        readExact(file, bytes);
        if (bytes[0] != 'A' || bytes[1] != 'D' || bytes[2] != 'K' || bytes[3] != 'H' || bytes[4] != 1) throw CalculatorError{"Unsupported calculator history format"};
        values.History.Count = bytes[5];
        for (std::size_t i = 0; i < values.History.Entries.size(); ++i) {
            for (std::size_t j = 0; j < values.History.Entries[i].size(); ++j) values.History.Entries[i][j] = static_cast<char>(bytes[6 + i * 80 + j]);
        }
        std::uint8_t extra = 0;
        const auto count = sceKernelRead(file, &extra, 1);
        if (count < 0) throw CalculatorError{"Read calculator history", static_cast<int>(count)};
        if (count != 0) throw CalculatorError{"Unexpected calculator history data"};
        validate(values);
    } catch (...) {
        check(sceKernelClose(file), "Close calculator history");
        throw;
    }
    check(sceKernelClose(file), "Close calculator history");
}

void writeFile(const char* path, const char* temporaryPath, std::span<const std::uint8_t> bytes) {
    const int file = check(sceKernelOpen(temporaryPath, SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_TRUNC, 0666), "Create calculator settings");
    try {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const auto count = sceKernelWrite(file, bytes.data() + offset, bytes.size() - offset);
            if (count < 0) throw CalculatorError{"Write calculator settings", static_cast<int>(count)};
            if (count == 0) throw CalculatorError{"Calculator settings write made no progress"};
            offset += static_cast<std::size_t>(count);
        }
        check(sceKernelFsync(file), "Flush calculator settings");
    } catch (...) {
        check(sceKernelClose(file), "Close calculator settings");
        throw;
    }
    check(sceKernelClose(file), "Close calculator settings");
    check(sceKernelRename(temporaryPath, path), "Replace calculator settings");
}

SaveDataMountResult mountSave(int userId, int resource) {
    const SceSaveDataDirName name{"Settings"};
    SaveDataMount3 request{};
    request.user_id = userId;
    request.dir_name = &name;
    request.blocks = 56;
    request.mount_mode = 32;
    request.resource = resource;
    SaveDataMountResult result{};
    check(sceSaveDataMount3(&request, &result), "sceSaveDataMount3");
    return result;
}

void makePath(char (&path)[64], const SaveDataMountPoint& mount, const char* filename) {
    std::size_t length = 0;
    while (length < sizeof(mount.data) && mount.data[length] != 0) ++length;
    if (length == 0 || length == sizeof(mount.data)) throw CalculatorError{"Invalid SaveData mount point"};
    const int count = snprintf_nid_postfix(path, sizeof(path), "%s/%s", mount.data, filename);
    if (count < 0 || static_cast<std::size_t>(count) >= sizeof(path)) throw CalculatorError{"SaveData path exceeds capacity"};
}

void writeSave(const SaveDataMountResult& mount, int resource, const Settings::Data& values) {
    SaveDataPrepareParam prepare{};
    prepare.resource = resource;
    check(sceSaveDataPrepare(&mount.mount_point, &prepare), "sceSaveDataPrepare");
    if ((mount.mount_status & 1u) != 0) {
        SaveDataParam param{};
        const char title[] = "AnyDevKit Calculator";
        for (std::size_t i = 0; i < sizeof(title); ++i) param.title[i] = title[i];
        check(sceSaveDataSetParam(&mount.mount_point, 0, &param, sizeof(param)), "sceSaveDataSetParam");
    }
    char path[64];
    char temporaryPath[64];
    makePath(path, mount.mount_point, "vibration.bin");
    makePath(temporaryPath, mount.mount_point, "vibration.tmp");
    const std::array<std::uint8_t, 8> vibration{'A', 'D', 'K', 'V', 3, static_cast<std::uint8_t>(values.Vibration[0]), static_cast<std::uint8_t>(values.Vibration[1]), static_cast<std::uint8_t>(values.Vibration[2])};
    writeFile(path, temporaryPath, vibration);
    std::array<std::uint8_t, 166> history{'A', 'D', 'K', 'H', 1, static_cast<std::uint8_t>(values.History.Count)};
    for (std::size_t i = 0; i < values.History.Count; ++i) {
        const auto& entry = values.History.Entries[i];
        for (std::size_t j = 0; j < entry.size() && entry[j] != 0; ++j) history[6 + i * 80 + j] = static_cast<std::uint8_t>(entry[j]);
    }
    makePath(path, mount.mount_point, "history.bin");
    makePath(temporaryPath, mount.mount_point, "history.tmp");
    writeFile(path, temporaryPath, history);
    SaveDataCommitParam commit{};
    commit.resource = resource;
    check(sceSaveDataCommit(&commit), "sceSaveDataCommit");
}

}

Settings::Settings(int userId) : userId(userId) {
    check(sceSysmoduleLoadModuleInternal(0x8000000f), "Load SaveData");
    check(sceSaveDataInitialize3(nullptr), "sceSaveDataInitialize3");
    try {
        resource = check(sceSaveDataCreateTransactionResource(786432), "sceSaveDataCreateTransactionResource");
    } catch (...) {
        check(sceSaveDataTerminate(), "sceSaveDataTerminate");
        throw;
    }
}

Settings::~Settings() noexcept(false) {
    check(sceSaveDataDeleteTransactionResource(resource), "sceSaveDataDeleteTransactionResource");
    check(sceSaveDataTerminate(), "sceSaveDataTerminate");
}

Settings::Data Settings::Load() {
    const auto mount = mountSave(userId, resource);
    Data values{};
    try {
        if ((mount.mount_status & 1u) != 0) {
            writeSave(mount, resource, values);
        } else {
            char path[64];
            makePath(path, mount.mount_point, "vibration.bin");
            values = readFile(path);
            makePath(path, mount.mount_point, "history.bin");
            readHistory(path, values);
        }
    } catch (...) {
        check(sceSaveDataUmount2(0, &mount.mount_point), "sceSaveDataUmount2");
        throw;
    }
    check(sceSaveDataUmount2(0, &mount.mount_point), "sceSaveDataUmount2");
    return values;
}

void Settings::Save(const Settings::Data& values) {
    validate(values);
    const auto mount = mountSave(userId, resource);
    try {
        writeSave(mount, resource, values);
    } catch (...) {
        check(sceSaveDataUmount2(0, &mount.mount_point), "sceSaveDataUmount2");
        throw;
    }
    check(sceSaveDataUmount2(0, &mount.mount_point), "sceSaveDataUmount2");
}
