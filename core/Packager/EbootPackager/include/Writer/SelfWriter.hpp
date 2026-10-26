#ifndef ANYDEVKIT_CORE_PACKAGER_EBOOTPACKAGER_INCLUDE_WRITER_SELFWRITER_HPP
#define ANYDEVKIT_CORE_PACKAGER_EBOOTPACKAGER_INCLUDE_WRITER_SELFWRITER_HPP

#include "Builder/SelfLayoutBuilder.hpp"
#include "EbootPackager.hpp"

namespace AnyDevKit::Self {

class SelfWriter final {
public:
    static std::vector<std::byte> Write(const ElfImage& image, const EbootPackageOptions& options);
};

}

#endif
