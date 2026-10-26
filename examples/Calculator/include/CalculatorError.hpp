#pragma once

struct CalculatorError final {
    const char* Message;
    int Code = 1;
};
