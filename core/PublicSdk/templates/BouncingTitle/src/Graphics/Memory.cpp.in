#include "Graphics/Memory.hpp"
#include "Error.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"

Memory::Memory(std::size_t size, std::uintptr_t hint) : size(size) {
    if (size == 0 || size % Alignment != 0) throw Error{"Invalid direct memory size"};
    Check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), size, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
    void* address = reinterpret_cast<void*>(hint);
    const int result = sceKernelMapDirectMemory(&address, size, 0x33, 0, physicalAddress, Alignment);
    if (result != 0) {
        Check(sceKernelReleaseDirectMemory(physicalAddress, size), "sceKernelReleaseDirectMemory");
        throw Error{"sceKernelMapDirectMemory", result};
    }

    data = static_cast<std::byte*>(address);
}

Memory::~Memory() noexcept(false) {
    Check(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(data), size), "sceKernelMunmap");
    Check(sceKernelReleaseDirectMemory(physicalAddress, size), "sceKernelReleaseDirectMemory");
}

void* Memory::Allocate(std::size_t bytes, std::size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0 || alignment > Alignment) throw Error{"Invalid GPU alignment"};
    const auto offset = (used + alignment - 1) & ~(alignment - 1);
    if (offset > size || bytes > size - offset) throw Error{"GPU memory exhausted"};
    used = offset + bytes;

    return data + offset;
}

void* Memory::Data() const { return data; }
std::size_t Memory::Size() const { return size; }

