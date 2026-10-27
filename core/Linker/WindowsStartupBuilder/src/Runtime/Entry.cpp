#include "Runtime/Entry.hpp"
#include "Runtime/PeRelocations.hpp"
#include "Runtime/StartupError.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"

#include <array>
#include <cstdint>
#include <new>

extern "C" {
extern void (*__CTOR_LIST__[])();
extern void (*__DTOR_LIST__[])();
}

namespace {

struct Destructor {
    void (*Callback)();
};

bool initialized = false;
std::array<std::uint64_t, 15> allocator{0x78, 2};
std::array<std::uint64_t, 7> libcParameters{0x38};
std::array<std::uint64_t, 8> processParameters{0x40, 0x4942524f};

void APS5_VABI InvokeDestructor(void* context) {
    auto* entry = static_cast<Destructor*>(context);
    const auto callback = entry->Callback;
    entry->~Destructor();
    free_nid_postfix(entry);
    callback();
}

void APS5_VABI Finalize() {
    for (auto* current = __DTOR_LIST__ + 1; *current; ++current) (*current)();
}

}

int AnyDevKitRegisterDestructor(void (*callback)()) {
    if (!callback) throw AnyDevKit::Windows::StartupError{"Null destructor callback"};
    auto* storage = malloc_nid_postfix(sizeof(Destructor));
    if (!storage) throw AnyDevKit::Windows::StartupError{"Cannot allocate destructor registration"};
    auto* entry = new (storage) Destructor{callback};
    if (__cxa_atexit_nid_postfix(InvokeDestructor, entry, nullptr) != 0) {
        entry->~Destructor();
        free_nid_postfix(entry);
        throw AnyDevKit::Windows::StartupError{"Cannot register destructor"};
    }
    return 0;
}

void __main() {
    if (initialized) return;
    initialized = true;
    if (atexit_nid_postfix(Finalize) != 0) throw AnyDevKit::Windows::StartupError{"Cannot register process finalization"};
    auto count = reinterpret_cast<std::uintptr_t>(__CTOR_LIST__[0]);
    if (count == ~std::uintptr_t{}) {
        count = 0;
        while (__CTOR_LIST__[count + 1]) ++count;
    }
    while (count) __CTOR_LIST__[count--]();
}

void AnyDevKitEntry() {
    libcParameters[6] = reinterpret_cast<std::uintptr_t>(allocator.data());
    processParameters[7] = reinterpret_cast<std::uintptr_t>(libcParameters.data());
    ApplicationHeapInitialize_nid_no_patch(processParameters.data());
    _pei386_runtime_relocator();
    __main();
    char* arguments[]{nullptr};
    exit_nid_postfix(AnyDevKitInvokeMain(0, arguments));
    throw AnyDevKit::Windows::StartupError{"Process exit returned"};
}
