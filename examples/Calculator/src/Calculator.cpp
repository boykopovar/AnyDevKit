#include "Calculator.hpp"
#include "CalculatorError.hpp"
#include "prx/libc/include/Api.hpp"

#include <algorithm>
#include <cmath>

namespace {

template<std::size_t TSize, typename... TArguments>
void format(std::array<char, TSize>& destination, const char* pattern, TArguments... arguments) {
    const auto count = snprintf_nid_postfix(destination.data(), destination.size(), pattern, arguments...);
    if (count < 0 || static_cast<std::size_t>(count) >= destination.size()) throw CalculatorError{"NUMBER TOO LARGE"};
}

}

std::string_view Calculator::Display() const { return entry.data(); }
std::string_view Calculator::Expression() const { return expression.data(); }
std::size_t Calculator::HistoryCount() const { return historyCount; }
bool Calculator::HasError() const { return error; }

std::string_view Calculator::History(std::size_t index) const {
    if (index >= historyCount) throw CalculatorError{"Invalid history index"};
    return history[index].data();
}

void Calculator::clear() {
    entry.fill(0);
    entry[0] = '0';
    expression.fill(0);
    accumulator = 0;
    repeatedOperand = 0;
    pending = 0;
    repeatedOperation = 0;
    awaitingOperand = false;
    completed = false;
    error = false;
}

double Calculator::value() const {
    char* end = nullptr;
    const auto result = strtod_nid_postfix(entry.data(), &end);
    if (end == entry.data() || *end != 0 || !std::isfinite(result)) throw CalculatorError{"INVALID NUMBER"};
    return result;
}

void Calculator::setValue(double number) {
    if (!std::isfinite(number)) throw CalculatorError{"NUMBER TOO LARGE"};
    format(entry, "%.12G", number == 0 ? 0.0 : number);
}

void Calculator::digit(char character) {
    if (completed) clear();
    if (awaitingOperand) {
        entry.fill(0);
        entry[0] = '0';
        awaitingOperand = false;
    }
    const auto text = Display();
    if (std::ranges::count_if(text, [](char c) { return c >= '0' && c <= '9'; }) >= 12) throw CalculatorError{"LIMIT 12 DIGITS"};
    if (text == "0") entry[0] = character;
    else if (text == "-0") entry[1] = character;
    else {
        if (text.size() + 1 >= entry.size()) throw CalculatorError{"NUMBER TOO LARGE"};
        entry[text.size()] = character;
        entry[text.size() + 1] = 0;
    }
}

void Calculator::updateExpression() {
    if (pending != 0) format(expression, "%.12G %c", accumulator, pending);
    else expression.fill(0);
}

void Calculator::addHistory(double left, char operation, double right) {
    for (std::size_t index = history.size() - 1; index != 0; --index) history[index] = history[index - 1];
    format(history[0], "%.12G %c %.12G = %s", left, operation, right, entry.data());
    historyCount = std::min(historyCount + 1, history.size());
}

void Calculator::calculate(double right, char operation) {
    const auto left = accumulator;
    double result = 0;
    switch (operation) {
    case '+': result = left + right; break;
    case '-': result = left - right; break;
    case '*': result = left * right; break;
    case '/':
        if (right == 0) throw CalculatorError{"DIVISION BY ZERO"};
        result = left / right;
        break;
    default: throw CalculatorError{"Invalid operation"};
    }
    setValue(result);
    accumulator = value();
    addHistory(left, operation, right);
}

void Calculator::operate(char operation) {
    if (pending != 0 && !awaitingOperand) calculate(value(), pending);
    else if (pending == 0) accumulator = value();
    pending = operation;
    awaitingOperand = true;
    completed = false;
    repeatedOperation = 0;
    updateExpression();
}

void Calculator::evaluate() {
    if (pending != 0) {
        repeatedOperand = value();
        repeatedOperation = pending;
        calculate(repeatedOperand, pending);
        pending = 0;
    } else if (completed && repeatedOperation != 0) calculate(repeatedOperand, repeatedOperation);
    if (historyCount != 0) format(expression, "%s", history[0].data());
    awaitingOperand = false;
    completed = true;
}

void Calculator::Press(CalculatorKey key) {
    if (error) clear();
    try {
        switch (key) {
        case CalculatorKey::Clear: clear(); break;
        case CalculatorKey::Zero: digit('0'); break;
        case CalculatorKey::One: digit('1'); break;
        case CalculatorKey::Two: digit('2'); break;
        case CalculatorKey::Three: digit('3'); break;
        case CalculatorKey::Four: digit('4'); break;
        case CalculatorKey::Five: digit('5'); break;
        case CalculatorKey::Six: digit('6'); break;
        case CalculatorKey::Seven: digit('7'); break;
        case CalculatorKey::Eight: digit('8'); break;
        case CalculatorKey::Nine: digit('9'); break;
        case CalculatorKey::Add: operate('+'); break;
        case CalculatorKey::Subtract: operate('-'); break;
        case CalculatorKey::Multiply: operate('*'); break;
        case CalculatorKey::Divide: operate('/'); break;
        case CalculatorKey::Equals: evaluate(); break;
        case CalculatorKey::Decimal:
            if (completed) clear();
            if (awaitingOperand) { entry.fill(0); entry[0] = '0'; awaitingOperand = false; }
            if (Display().find('.') == std::string_view::npos && Display().find('E') == std::string_view::npos) {
                const auto length = Display().size();
                if (length + 1 >= entry.size()) throw CalculatorError{"NUMBER TOO LARGE"};
                entry[length] = '.';
                entry[length + 1] = 0;
            }
            break;
        case CalculatorKey::Backspace:
            if (completed || awaitingOperand) { entry.fill(0); entry[0] = '0'; completed = false; awaitingOperand = false; }
            else if (Display().find('E') != std::string_view::npos) throw CalculatorError{"PRESS AC TO CLEAR"};
            else {
                entry[Display().size() - 1] = 0;
                if (Display().empty() || Display() == "-") { entry.fill(0); entry[0] = '0'; }
            }
            break;
        case CalculatorKey::Sign:
            if (Display().front() == '-') std::move(entry.begin() + 1, entry.end(), entry.begin());
            else {
                const auto length = Display().size();
                if (length + 1 >= entry.size()) throw CalculatorError{"NUMBER TOO LARGE"};
                std::move_backward(entry.begin(), entry.begin() + length + 1, entry.begin() + length + 2);
                entry[0] = '-';
            }
            awaitingOperand = false;
            if (completed) accumulator = value();
            break;
        case CalculatorKey::Percent:
            setValue((pending == '+' || pending == '-') ? accumulator * value() / 100 : value() / 100);
            awaitingOperand = false;
            if (completed) accumulator = value();
            break;
        }
    } catch (const CalculatorError& failure) {
        clear();
        error = true;
        format(entry, "%s", failure.Message);
    }
}
