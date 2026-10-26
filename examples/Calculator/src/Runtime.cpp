#include "Runtime.hpp"
#include "PeRelocations.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"

#include <array>
#include <cstdint>

void InitializeRuntime() {
    _pei386_runtime_relocator();
    std::array<std::uint64_t, 15> allocator{0x78, 2};
    std::array<std::uint64_t, 7> libcParameters{0x38};
    std::array<std::uint64_t, 8> processParameters{0x40, 0x4942524f};
    libcParameters[6] = reinterpret_cast<std::uintptr_t>(allocator.data());
    processParameters[7] = reinterpret_cast<std::uintptr_t>(libcParameters.data());
    ApplicationHeapInitialize_nid_no_patch(processParameters.data());
}
