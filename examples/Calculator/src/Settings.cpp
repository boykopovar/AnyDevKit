#include "Settings.hpp"
#include "CalculatorError.hpp"
#include "prx/libkernel/File/include/File.hpp"
#include "prx/libkernel/File/include/FileFlags.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libSceSaveData.native/libSceSaveData.native.hpp"
#include "prx/libSceSysmodule/libSceSysmodule.hpp"
#include "prx/libc/include/Api.hpp"

#include <cstdint>

namespace {

int check(int result, const char* operation) {
    if (result < 0) throw CalculatorError{operation, result};
    return result;
}

void validate(const std::array<int, 3>& values) {
    if (values[0] < 0 || values[0] > 200 || values[0] % 5 != 0 || values[1] < 0 || values[1] > 255 || values[2] < 0 || values[2] > 255) throw CalculatorError{"Invalid vibration settings"};
}

std::array<int, 3> readFile(const char* path) {
    const int file = sceKernelOpen(path, SCE_KERNEL_O_RDONLY, 0);
    check(file, "Open vibration settings");
    std::array<std::uint8_t, 8> bytes{};
    try {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const auto count = sceKernelRead(file, bytes.data() + offset, bytes.size() - offset);
            if (count < 0) throw CalculatorError{"Read vibration settings", static_cast<int>(count)};
            if (count == 0) throw CalculatorError{"Truncated vibration settings"};
            offset += static_cast<std::size_t>(count);
        }
        std::uint8_t extra = 0;
        const auto count = sceKernelRead(file, &extra, 1);
        if (count < 0) throw CalculatorError{"Read vibration settings", static_cast<int>(count)};
        if (count != 0) throw CalculatorError{"Unexpected vibration settings data"};
    } catch (...) {
        check(sceKernelClose(file), "Close vibration settings");
        throw;
    }
    check(sceKernelClose(file), "Close vibration settings");
    if (bytes[0] != 'A' || bytes[1] != 'D' || bytes[2] != 'K' || bytes[3] != 'V' || bytes[4] != 1) throw CalculatorError{"Unsupported vibration settings format"};
    const std::array<int, 3> values{bytes[5], bytes[6], bytes[7]};
    validate(values);
    return values;
}

void writeFile(const char* path, const char* temporaryPath, const std::array<int, 3>& values) {
    validate(values);
    const std::array<std::uint8_t, 8> bytes{'A', 'D', 'K', 'V', 1, static_cast<std::uint8_t>(values[0]), static_cast<std::uint8_t>(values[1]), static_cast<std::uint8_t>(values[2])};
    const int file = check(sceKernelOpen(temporaryPath, SCE_KERNEL_O_WRONLY | SCE_KERNEL_O_CREAT | SCE_KERNEL_O_TRUNC, 0666), "Create vibration settings");
    try {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const auto count = sceKernelWrite(file, bytes.data() + offset, bytes.size() - offset);
            if (count < 0) throw CalculatorError{"Write vibration settings", static_cast<int>(count)};
            if (count == 0) throw CalculatorError{"Vibration settings write made no progress"};
            offset += static_cast<std::size_t>(count);
        }
        check(sceKernelFsync(file), "Flush vibration settings");
    } catch (...) {
        check(sceKernelClose(file), "Close vibration settings");
        throw;
    }
    check(sceKernelClose(file), "Close vibration settings");
    check(sceKernelRename(temporaryPath, path), "Replace vibration settings");
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

void writeSave(const SaveDataMountResult& mount, int resource, const std::array<int, 3>& values) {
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
    writeFile(path, temporaryPath, values);
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

std::array<int, 3> Settings::Load() {
    const auto mount = mountSave(userId, resource);
    std::array<int, 3> values{};
    try {
        if ((mount.mount_status & 1u) != 0) {
            values = {55, 1, 1};
            writeSave(mount, resource, values);
        } else {
            char path[64];
            makePath(path, mount.mount_point, "vibration.bin");
            values = readFile(path);
        }
    } catch (...) {
        check(sceSaveDataUmount2(0, &mount.mount_point), "sceSaveDataUmount2");
        throw;
    }
    check(sceSaveDataUmount2(0, &mount.mount_point), "sceSaveDataUmount2");
    return values;
}

void Settings::Save(const std::array<int, 3>& values) {
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
