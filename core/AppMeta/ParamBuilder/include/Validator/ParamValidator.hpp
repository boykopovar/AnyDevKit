#ifndef ANYDEVKIT_PARAM_VALIDATOR_HPP
#define ANYDEVKIT_PARAM_VALIDATOR_HPP

#include "ParamBuilder.hpp"

namespace AnyDevKit::Param {

class ParamValidator final {
public:
    static void Validate(const ApplicationParameters& parameters);
};

}

#endif
