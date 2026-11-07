#include "Error.hpp"

int Check(int result, const char* operation) {
    if (result < 0) throw Error{operation, result};
    return result;
}

