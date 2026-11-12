#include "Imports/ImportCatalog.hpp"
#include <stdexcept>
#include <array>

namespace {

void require(bool condition) {
    if (!condition) throw std::runtime_error("Import catalog check failed");
}

template<typename TException, typename TAction>
void requireError(TAction action) {
    try {
        action();
    } catch (const TException&) {
        return;
    }

    throw std::runtime_error("Expected import catalog error");
}

}

int main() {
    AnyDevKit::ImportCatalog catalog;
    requireError<std::out_of_range>([&] { catalog.AddModule("missing.sprx"); });
    require(catalog.Modules().empty());
    catalog.AddModule("libkernel.sprx");
    requireError<std::invalid_argument>([&] { catalog.AddModule("libkernel.sprx"); });
    const auto openPsId = catalog.Resolve("sceKernelGetOpenPsId");
    require(openPsId.Nid == "DLORcroUqbc");
    require(openPsId.Module == "libkernel" && openPsId.Library == "libSceOpenPsId");
    require(openPsId.ModuleVersion == 0 && openPsId.LibraryVersion == 0);
    requireError<std::invalid_argument>([&] { catalog.Resolve("adk_missing_export"); });
    requireError<std::invalid_argument>([&] { catalog.Resolve("pthread_create"); });
    requireError<std::invalid_argument>([&] { catalog.Select("pthread_create", "libkernel", "missing"); });
    catalog.Select("pthread_create", "libkernel", "libScePosix");
    require(catalog.Resolve("pthread_create").Library == "libScePosix");
    catalog.Select("pthread_create", "libkernel", "libkernel");
    require(catalog.Resolve("pthread_create").Library == "libkernel");
    catalog.AddModule("libSceLibcInternal.sprx");
    const auto allocator = catalog.Resolve("_malloc_init");
    require(allocator.Module == "libSceLibcInternal");
    require(allocator.FileName == "libSceLibcInternal.sprx");
    require(allocator.ModuleVersion == 0 && allocator.LibraryVersion == 0);
    requireError<std::invalid_argument>([&] { catalog.AddModule("libkernel_sys.sprx"); });
    requireError<std::logic_error>([&] { catalog.SelectModule("libkernel", "libkernel_sys.sprx"); });
    AnyDevKit::ImportCatalog selected;
    requireError<std::invalid_argument>([&] { selected.SelectModule("libkernel", "libSceLibcInternal.sprx"); });
    selected.SelectModule("libkernel", "libkernel_sys.sprx");
    requireError<std::invalid_argument>([&] { selected.SelectModule("libkernel", "libkernel.sprx"); });
    requireError<std::invalid_argument>([&] { selected.ValidateDependencies({}); });
    selected.AddModule("libkernel.sprx");
    selected.AddModule("libkernel_sys.sprx");
    const std::array<std::string, 2> dependencies{"libkernel.sprx", "libkernel_sys.sprx"};
    selected.ValidateDependencies(dependencies);
    require(selected.Modules().size() == 1);
    require(selected.Resolve("sceKernelGetOpenPsId").FileName == "libkernel_sys.sprx");
    requireError<std::invalid_argument>([&] { selected.AddModule("libkernel.sprx"); });
    const std::array<std::string, 2> mismatch{"libkernel.sprx", "libkernel_web.sprx"};
    requireError<std::invalid_argument>([&] { selected.ValidateDependencies(mismatch); });

}

