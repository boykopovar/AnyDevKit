#include "PeRelocations.hpp"
#include "prx/libc/include/Api.hpp"

#include <cstddef>
#include <cstdint>

extern "C" {
extern const std::byte __ImageBase[];
extern const std::uint32_t __RUNTIME_PSEUDO_RELOC_LIST__[];
extern const std::uint32_t __RUNTIME_PSEUDO_RELOC_LIST_END__[];
}

void _pei386_runtime_relocator() {
    const auto* cursor = __RUNTIME_PSEUDO_RELOC_LIST__;
    const auto* end = __RUNTIME_PSEUDO_RELOC_LIST_END__;
    if (cursor == end) return;
    if (end - cursor < 3 || cursor[0] != 0 || cursor[1] != 0 || cursor[2] != 1 || (end - cursor) % 3 != 0) {
        printf_nid_postfix("Unsupported PE relocation table\n");
        exit_nid_postfix(1);
        return;
    }
    const auto base = reinterpret_cast<std::uintptr_t>(__ImageBase);
    for (cursor += 3; cursor != end; cursor += 3) {
        if (cursor[2] != 64) {
            printf_nid_postfix("Unsupported PE relocation width\n");
            exit_nid_postfix(1);
            return;
        }
        const auto symbol = base + cursor[0];
        auto* target = reinterpret_cast<std::uintptr_t*>(base + cursor[1]);
        *target += *reinterpret_cast<const std::uintptr_t*>(symbol) - symbol;
    }
}
