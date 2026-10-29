#include "Calculator.hpp"
#include "CalculatorError.hpp"
#include "prx/libc/include/Api.hpp"

#include <algorithm>
#include <cmath>
#include <span>
#include <numeric>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {

struct Digit { char Value; };
struct Operation { char Value; };
using Action = std::variant<Digit, Operation, CalculatorKey>;

consteval auto buildActions() {
    std::vector<std::pair<CalculatorKey, Action>> definitions{
        {CalculatorKey::Clear, CalculatorKey::Clear},
        {CalculatorKey::Sign, CalculatorKey::Sign},
        {CalculatorKey::Percent, CalculatorKey::Percent},
        {CalculatorKey::Divide, Operation{'/'}},
        {CalculatorKey::Multiply, Operation{'*'}},
        {CalculatorKey::Subtract, Operation{'-'}},
        {CalculatorKey::Add, Operation{'+'}},
        {CalculatorKey::Backspace, CalculatorKey::Backspace},
        {CalculatorKey::Decimal, CalculatorKey::Decimal},
        {CalculatorKey::Equals, CalculatorKey::Equals}
    };
    constexpr auto digits = std::to_array<CalculatorKey>({CalculatorKey::Zero, CalculatorKey::One, CalculatorKey::Two, CalculatorKey::Three, CalculatorKey::Four, CalculatorKey::Five, CalculatorKey::Six, CalculatorKey::Seven, CalculatorKey::Eight, CalculatorKey::Nine});
    for (std::size_t index = 0; index < digits.size(); ++index) definitions.emplace_back(digits[index], Digit{static_cast<char>('0' + index)});
    std::array<std::size_t, static_cast<std::size_t>(CalculatorKey::Equals) + 1> order{};
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::ranges::sort(order, {}, [&](std::size_t index) { return definitions[index].first; });
    std::array<Action, static_cast<std::size_t>(CalculatorKey::Equals) + 1> actions{};
    if (definitions.size() != actions.size()) throw "Incomplete calculator actions";
    for (std::size_t index = 0; index < actions.size(); ++index) {
        if (static_cast<std::size_t>(definitions[order[index]].first) != index) throw "Invalid calculator action ordering";
        actions[index] = definitions[order[index]].second;
    }
    return actions;
}

constexpr auto Actions = buildActions();

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
CalculatorHistory Calculator::GetHistory() const { return {history, historyCount}; }

void Calculator::RestoreHistory(const CalculatorHistory& saved) {
    if (saved.Count > saved.Entries.size()) throw CalculatorError{"Invalid saved history count"};
    for (std::size_t i = 0; i < saved.Count; ++i) {
        const auto& text = saved.Entries[i];
        if (text[0] == 0 || std::ranges::find(text, 0) == text.end()) throw CalculatorError{"Invalid saved history text"};
    }
    history = saved.Entries;
    historyCount = saved.Count;
}

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
    std::ranges::rotate(history, history.end() - 1);
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
        const auto index = static_cast<std::size_t>(key);
        const std::span<const Action> actions = Actions;
        if (index >= actions.size()) throw CalculatorError{"Invalid calculator key"};
        std::visit([this](const auto& action) {
            using TAction = std::remove_cvref_t<decltype(action)>;
            if constexpr (std::is_same_v<TAction, Digit>) digit(action.Value);
            else if constexpr (std::is_same_v<TAction, Operation>) operate(action.Value);
            else command(action);
        }, actions[index]);
    } catch (const CalculatorError& failure) {
        clear();
        error = true;
        format(entry, "%s", failure.Message);
    }
}

void Calculator::command(CalculatorKey key) {
    switch (key) {
    case CalculatorKey::Clear: clear(); break;
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
    default: throw CalculatorError{"Invalid calculator command"};
    }
}
