#ifndef ANYDEVKIT_WINDOWSSTARTUPBUILDER_HPP
#define ANYDEVKIT_WINDOWSSTARTUPBUILDER_HPP

#include <cstddef>
#include <filesystem>
#include <vector>

namespace AnyDevKit {

class WindowsStartupBuilder final {
public:
    static std::vector<std::byte> Build();
    static void WriteFile(const std::filesystem::path& output);
};

}

#endif
