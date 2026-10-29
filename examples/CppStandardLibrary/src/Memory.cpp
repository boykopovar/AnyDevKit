#include "Checks.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string>
#include <vector>

void CheckMemory() {
    const auto handler = +[] { throw std::bad_alloc(); };
    const auto previousHandler = std::set_new_handler(handler);
    const auto activeHandler = std::get_new_handler();
    std::set_new_handler(previousHandler);
    Require(activeHandler == handler, "Allocation handler");
    struct alignas(64) Value { int Data = 42; };
    auto unique = std::make_unique<Value>();
    Require(reinterpret_cast<std::uintptr_t>(unique.get()) % 64 == 0 && unique->Data == 42, "Aligned unique pointer");
    auto array = std::make_unique<int[]>(128); array[127] = 9;
    Require(array[127] == 9 && array[0] == 0, "Unique array");
    int deleted = 0;
    std::weak_ptr<int> weak;
    {
        std::shared_ptr<int> shared(new int(7), [&](int* value) { ++deleted; delete value; });
        weak = shared;
        auto copied = weak.lock();
        Require(copied && *copied == 7 && copied.use_count() == 2, "Shared ownership");
        shared.reset(); Require(!weak.expired(), "Weak lifetime");
    }
    Require(deleted == 1 && weak.expired() && !weak.lock(), "Shared deletion");
    ExpectException<std::bad_weak_ptr>([&] { std::shared_ptr<int> invalid(weak); });
    struct Owner : std::enable_shared_from_this<Owner> {};
    auto owner = std::make_shared<Owner>();
    Require(owner->shared_from_this() == owner, "Shared from this");
    auto storage = std::make_shared<std::array<int, 2>>(std::array{3, 4});
    std::shared_ptr<int> alias(storage, &(*storage)[1]); storage.reset();
    Require(*alias == 4, "Aliasing pointer");
    std::array<std::byte, 8192> buffer{};
    std::pmr::monotonic_buffer_resource resource(buffer.data(), buffer.size(), std::pmr::null_memory_resource());
    std::pmr::vector<int> values(&resource); values.resize(128, 17);
    Require(values.back() == 17, "Polymorphic allocator");
    ExpectException<std::bad_alloc>([] { static_cast<void>(std::pmr::null_memory_resource()->allocate(16)); });
    std::allocator<int> allocator; auto* raw = allocator.allocate(1);
    std::construct_at(raw, 13); Require(*raw == 13, "Construct at");
    std::destroy_at(raw); allocator.deallocate(raw, 1);
}
