#ifndef ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_MEMORY_HPP
#define ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_MEMORY_HPP
#include <cstddef>
#include <cstdint>

class Memory final {
public:
    static constexpr std::size_t Alignment = 131072;
    Memory(std::size_t size, std::uintptr_t hint = 0);
    ~Memory() noexcept(false);
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;
    void* Allocate(std::size_t bytes, std::size_t alignment);
    void* Data() const;
    std::size_t Size() const;

private:
    std::byte* data = nullptr;
    std::int64_t physicalAddress = 0;
    std::size_t size;
    std::size_t used = 0;
};
#endif

