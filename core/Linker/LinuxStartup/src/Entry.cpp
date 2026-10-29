#include "Entry.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include <array>
#include <cstdint>
extern "C" {
extern void (*__init_array_start[])();
extern void (*__init_array_end[])();
extern void (*__fini_array_start[])();
extern void (*__fini_array_end[])();
void* __dso_handle = &__dso_handle;
}
namespace {
void finalize() {
    auto current = __fini_array_end;
    while (current != __fini_array_start) (*--current)();
}
}
void AnyDevKitLinuxEntry(int argc, char** argv) {
    static std::array<std::uint64_t, 15> allocator{0x78, 2};
    static std::array<std::uint64_t, 7> libcParameters{0x38};
    static std::array<std::uint64_t, 8> processParameters{0x40, 0x4942524f};
    libcParameters[6] = reinterpret_cast<std::uintptr_t>(allocator.data());
    processParameters[7] = reinterpret_cast<std::uintptr_t>(libcParameters.data());
    ApplicationHeapInitialize_nid_no_patch(processParameters.data());
    if (atexit_nid_postfix(finalize) != 0) throw "Cannot register process finalization";
    for (auto current = __init_array_start; current != __init_array_end; ++current) (*current)();
    exit_nid_postfix(AnyDevKitLinuxMain(argc, argv));
    throw "Process exit returned";
}
