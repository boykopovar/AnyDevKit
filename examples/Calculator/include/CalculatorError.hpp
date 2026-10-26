#ifndef ANYDEVKIT_EXAMPLES_CALCULATOR_INCLUDE_CALCULATORERROR_HPP
#define ANYDEVKIT_EXAMPLES_CALCULATOR_INCLUDE_CALCULATORERROR_HPP

struct CalculatorError final {
    const char* Message;
    int Code = 1;
};

#endif
