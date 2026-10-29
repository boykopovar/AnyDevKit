#include "Checks.hpp"
#include <atomic>

void CheckAtomics() {
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    std::atomic<unsigned> value{2};
    Require(value.fetch_add(3) == 2 && value.load() == 5, "Atomic arithmetic");
    unsigned expected = 5;
    Require(value.compare_exchange_strong(expected, 7) && value.load() == 7, "Atomic compare exchange");
    alignas(std::atomic_ref<unsigned>::required_alignment) unsigned storage = 1;
    std::atomic_ref<unsigned> reference(storage); reference.store(9);
    Require(storage == 9, "Atomic reference");
    std::atomic_flag flag{};
    Require(!flag.test_and_set() && flag.test(), "Atomic flag"); flag.clear();
}
