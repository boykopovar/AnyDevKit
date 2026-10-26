#pragma once

#include "Builder/SelfLayoutBuilder.hpp"
#include "EbootPackager.hpp"

namespace AnyDevKit::Self {

class SelfWriter final {
public:
    static std::vector<std::byte> Write(const ElfImage& image, const EbootPackageOptions& options);
};

}
