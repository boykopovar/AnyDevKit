#ifndef ANYDEVKIT_PARAM_WRITER_HPP
#define ANYDEVKIT_PARAM_WRITER_HPP

#include "ParamBuilder.hpp"

namespace AnyDevKit::Param {

class ParamWriter final {
public:
    static std::string Write(const ApplicationParameters& parameters);
};

}

#endif
