#ifndef ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_ERROR_HPP
#define ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_ERROR_HPP

#include "Calculator3DError.hpp"

inline int Check(int result, const char* operation) {
    if (result < 0) throw Calculator3DError{operation, result};
    return result;
}

#endif

