#include "Imports/ImportCatalog.hpp"
#include "Builder/WeakSymbolBuilder.hpp"
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

    using AnyDevKit::Ps5::Symbol;
    using AnyDevKit::Ps5::Relocation;
    using AnyDevKit::Ps5::WeakSymbolBuilder;
    const std::vector<Symbol> originalSymbols{{}, {"adk_missing_weak", 0x20, 0, 0, 0, 0}, {"sceKernelGetOpenPsId", 0x12, 0, 0, 0, 0}, {"adk_defined_weak", 0x22, 0, 1, 0x1000, 0}};
    auto symbols = originalSymbols;
    std::vector<Relocation> relocations{{0x100, (1ull << 32) | 1, 42}, {0x108, (1ull << 32) | 6, 0}, {0x110, (2ull << 32) | 6, 0}, {0x118, 8, 0x1000}};
    std::vector<Relocation> plt{{0x120, (1ull << 32) | 7, 0}};
    const auto writes = WeakSymbolBuilder::Resolve(selected, symbols, relocations, plt);
    require(writes.size() == 3 && writes.at(0x100) == 42 && writes.at(0x108) == 0 && writes.at(0x120) == 0);
    require(plt.empty() && symbols.size() == 3 && symbols[2].Name == "adk_defined_weak");
    require(relocations.size() == 2 && relocations[0].Info == ((1ull << 32) | 6) && relocations[1].Info == 8);
    symbols = originalSymbols;
    relocations = {{0x100, (1ull << 32) | 16, 0}};
    requireError<std::invalid_argument>([&] { WeakSymbolBuilder::Resolve(selected, symbols, relocations, plt); });
    symbols = originalSymbols;
    symbols[1].Info = 0x26;
    relocations.clear();
    requireError<std::invalid_argument>([&] { WeakSymbolBuilder::Resolve(selected, symbols, relocations, plt); });
    symbols = originalSymbols;
    symbols[1].Info = 0x10;
    WeakSymbolBuilder::Resolve(selected, symbols, relocations, plt);
    require(symbols.size() == originalSymbols.size());
    requireError<std::invalid_argument>([&] { selected.Resolve(symbols[1].Name); });

    symbols = {{}, {"sceKernelGetOpenPsId", 0x22, 2, 0, 0, 0}};
    relocations = {{0x100, (1ull << 32) | 6, 0}};
    const auto hiddenWrites = WeakSymbolBuilder::Resolve(selected, symbols, relocations, plt);
    require(hiddenWrites.at(0x100) == 0 && symbols.size() == 1 && relocations.empty());

}

