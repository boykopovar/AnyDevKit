#include "Calculator.hpp"
#include "CalculatorError.hpp"
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <cmath>
#include <variant>
#include <type_traits>
#include <utility>

namespace {

constexpr std::size_t EntryCapacity = 32;
constexpr std::size_t TextCapacity = 80;
constexpr std::size_t HistoryCapacity = 2;
struct Digit { char Value; };
struct Operation { char Value; };
using Action = std::variant<Digit, Operation, CalculatorKey>;
constexpr auto Actions = std::to_array<Action>({
    CalculatorKey::Clear, CalculatorKey::Sign, CalculatorKey::Percent, Operation{'/'},
    Digit{'7'}, Digit{'8'}, Digit{'9'}, Operation{'*'},
    Digit{'4'}, Digit{'5'}, Digit{'6'}, Operation{'-'},
    Digit{'1'}, Digit{'2'}, Digit{'3'}, Operation{'+'},
    CalculatorKey::Backspace, Digit{'0'}, CalculatorKey::Decimal, CalculatorKey::Equals
});

template<std::size_t TSize, typename... TArguments>
std::string format(const char* pattern, TArguments... arguments) {
    std::array<char, TSize> buffer{};
    const auto count = std::snprintf(buffer.data(), buffer.size(), pattern, arguments...);
    if (count < 0 || static_cast<std::size_t>(count) >= buffer.size()) throw CalculatorError{"NUMBER TOO LARGE"};
    return {buffer.data(), static_cast<std::size_t>(count)};
}

}

Calculator::Calculator() {
    entry.reserve(EntryCapacity - 1);
    expression.reserve(TextCapacity - 1);
    history.reserve(HistoryCapacity);
}

std::string_view Calculator::Display() const { return entry; }
std::string_view Calculator::Expression() const { return expression; }
std::size_t Calculator::HistoryCount() const { return history.size(); }
bool Calculator::HasError() const { return error; }
CalculatorHistory Calculator::GetHistory() const {
    CalculatorHistory saved{};
    saved.Count = history.size();
    for (std::size_t i = 0; i < history.size(); ++i) std::ranges::copy(history[i], saved.Entries[i].begin());
    return saved;
}

void Calculator::RestoreHistory(const CalculatorHistory& saved) {
    if (saved.Count > saved.Entries.size()) throw CalculatorError{"Invalid saved history count"};
    std::vector<std::string> restored;
    restored.reserve(HistoryCapacity);
    for (std::size_t i = 0; i < saved.Count; ++i) {
        const auto& text = saved.Entries[i];
        const auto end = std::ranges::find(text, 0);
        if (end == text.begin() || end == text.end()) throw CalculatorError{"Invalid saved history text"};
        restored.emplace_back(text.begin(), end);
    }
    history.swap(restored);
}

std::string_view Calculator::History(std::size_t index) const {
    if (index >= history.size()) throw CalculatorError{"Invalid history index"};
    return history[index];
}

void Calculator::clear() {
    entry = "0";
    expression.clear();
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
    const auto result = std::strtod(entry.data(), &end);
    if (end == entry.data() || *end != 0 || !std::isfinite(result)) throw CalculatorError{"INVALID NUMBER"};
    return result;
}

void Calculator::setValue(double number) {
    if (!std::isfinite(number)) throw CalculatorError{"NUMBER TOO LARGE"};
    entry = format<EntryCapacity>("%.12G", number == 0 ? 0.0 : number);
}

void Calculator::digit(char character) {
    if (completed) clear();
    if (awaitingOperand) {
        entry = "0";
        awaitingOperand = false;
    }
    const auto text = Display();
    if (std::ranges::count_if(text, [](char c) { return c >= '0' && c <= '9'; }) >= 12) throw CalculatorError{"LIMIT 12 DIGITS"};
    if (text == "0") entry[0] = character;
    else if (text == "-0") entry[1] = character;
    else {
        if (text.size() + 1 >= EntryCapacity) throw CalculatorError{"NUMBER TOO LARGE"};
        entry.push_back(character);
    }
}

void Calculator::updateExpression() {
    if (pending != 0) expression = format<TextCapacity>("%.12G %c", accumulator, pending);
    else expression.clear();
}

void Calculator::addHistory(double left, char operation, double right) {
    auto text = format<TextCapacity>("%.12G %c %.12G = %s", left, operation, right, entry.c_str());
    if (history.size() == HistoryCapacity) history.pop_back();
    history.insert(history.begin(), std::move(text));
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
    if (!history.empty()) expression = history.front();
    awaitingOperand = false;
    completed = true;
}

void Calculator::Press(CalculatorKey key) {
    if (error) clear();
    try {
        const auto index = static_cast<std::size_t>(key);
        if (index >= Actions.size()) throw CalculatorError{"Invalid calculator key"};
        std::visit([this](const auto& action) {
            using TAction = std::remove_cvref_t<decltype(action)>;
            if constexpr (std::is_same_v<TAction, Digit>) digit(action.Value);
            else if constexpr (std::is_same_v<TAction, Operation>) operate(action.Value);
            else command(action);
        }, Actions[index]);
    } catch (const CalculatorError& failure) {
        clear();
        error = true;
        entry = format<EntryCapacity>("%s", failure.Message);
    }
}

void Calculator::command(CalculatorKey key) {
    switch (key) {
        case CalculatorKey::Clear: clear(); break;
        case CalculatorKey::Equals: evaluate(); break;
        case CalculatorKey::Decimal:
            if (completed) clear();
            if (awaitingOperand) { entry = "0"; awaitingOperand = false; }
            if (Display().find('.') == std::string_view::npos && Display().find('E') == std::string_view::npos) {
                const auto length = Display().size();
                if (length + 1 >= EntryCapacity) throw CalculatorError{"NUMBER TOO LARGE"};
                entry.push_back('.');
            }
            break;
        case CalculatorKey::Backspace:
            if (completed || awaitingOperand) { entry = "0"; completed = false; awaitingOperand = false; }
            else if (Display().find('E') != std::string_view::npos) throw CalculatorError{"PRESS AC TO CLEAR"};
            else {
                entry.pop_back();
                if (Display().empty() || Display() == "-") { entry = "0"; }
            }
            break;
        case CalculatorKey::Sign:
            if (Display().front() == '-') entry.erase(0, 1);
            else {
                const auto length = Display().size();
                if (length + 1 >= EntryCapacity) throw CalculatorError{"NUMBER TOO LARGE"};
                entry.insert(entry.begin(), '-');
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
