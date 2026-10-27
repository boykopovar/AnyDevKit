#ifndef ANYDEVKIT_LIBC_INITIALIZATION_HPP
#define ANYDEVKIT_LIBC_INITIALIZATION_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace AnyDevKit::Libc {

class Initialization final {
public:
    static std::vector<std::byte> Build(std::uint64_t address, std::uint64_t mallocInitSlot);
};

}

#endif
