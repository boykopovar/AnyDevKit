#ifndef ANYDEVKIT_PFS_FLATPATHWRITER_HPP
#define ANYDEVKIT_PFS_FLATPATHWRITER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace AnyDevKit::Pfs {

class FlatPathWriter final {
public:
    static std::uint64_t Hash(std::string_view path);
    static std::vector<std::byte> Build(std::span<const std::pair<std::uint64_t, std::uint64_t>> entries);
};

}

#endif
