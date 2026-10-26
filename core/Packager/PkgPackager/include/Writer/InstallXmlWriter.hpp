#ifndef ANYDEVKIT_PKG_INSTALLXMLWRITER_HPP
#define ANYDEVKIT_PKG_INSTALLXMLWRITER_HPP

#include "Builder/CntBuilder.hpp"
#include "Builder/InnerImageBuilder.hpp"
#include "ParamBuilder.hpp"

namespace AnyDevKit::Pkg {

class InstallXmlWriter final {
public:
    static std::string Build(const ApplicationParameters& parameters, const PackageKeys& keys, const InnerImageInfo& inner, const OuterImageInfo& outer, const CntImage& container);
};

}

#endif
