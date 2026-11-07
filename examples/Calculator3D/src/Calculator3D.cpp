#include "Calculator3D.hpp"
#include "Calculator3DError.hpp"
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
using Action = std::variant<Digit, Operation, Calculator3DKey>;
constexpr auto Actions = std::to_array<Action>({
    Calculator3DKey::Clear, Calculator3DKey::Sign, Calculator3DKey::Percent, Operation{'/'},
    Digit{'7'}, Digit{'8'}, Digit{'9'}, Operation{'*'},
    Digit{'4'}, Digit{'5'}, Digit{'6'}, Operation{'-'},
    Digit{'1'}, Digit{'2'}, Digit{'3'}, Operation{'+'},
    Calculator3DKey::Backspace, Digit{'0'}, Calculator3DKey::Decimal, Calculator3DKey::Equals
});

template<std::size_t TSize, typename... TArguments>
std::string format(const char* pattern, TArguments... arguments) {
    std::array<char, TSize> buffer{};
    const auto count = std::snprintf(buffer.data(), buffer.size(), pattern, arguments...);
    if (count < 0 || static_cast<std::size_t>(count) >= buffer.size()) throw Calculator3DError{"NUMBER TOO LARGE"};
    return {buffer.data(), static_cast<std::size_t>(count)};
}

}

Calculator3D::Calculator3D() {
    entry.reserve(EntryCapacity - 1);
    expression.reserve(TextCapacity - 1);
    history.reserve(HistoryCapacity);
}

std::string_view Calculator3D::Display() const { return entry; }
std::string_view Calculator3D::Expression() const { return expression; }
std::size_t Calculator3D::HistoryCount() const { return history.size(); }
bool Calculator3D::HasError() const { return error; }
Calculator3DHistory Calculator3D::GetHistory() const {
    Calculator3DHistory saved{};
    saved.Count = history.size();

    for (std::size_t i = 0; i < history.size(); ++i) std::ranges::copy(history[i], saved.Entries[i].begin());
    return saved;
}

void Calculator3D::RestoreHistory(const Calculator3DHistory& saved) {
    if (saved.Count > saved.Entries.size()) throw Calculator3DError{"Invalid saved history count"};
    std::vector<std::string> restored;
    restored.reserve(HistoryCapacity);

    for (std::size_t i = 0; i < saved.Count; ++i) {
        const auto& text = saved.Entries[i];
        const auto end = std::ranges::find(text, 0);
        if (end == text.begin() || end == text.end()) throw Calculator3DError{"Invalid saved history text"};
        restored.emplace_back(text.begin(), end);
    }

    history.swap(restored);
}

std::string_view Calculator3D::History(std::size_t index) const {
    if (index >= history.size()) throw Calculator3DError{"Invalid history index"};
    return history[index];
}

void Calculator3D::clear() {
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

double Calculator3D::value() const {
    char* end = nullptr;
    const auto result = std::strtod(entry.data(), &end);
    if (end == entry.data() || *end != 0 || !std::isfinite(result)) throw Calculator3DError{"INVALID NUMBER"};
    return result;
}

void Calculator3D::setValue(double number) {
    if (!std::isfinite(number)) throw Calculator3DError{"NUMBER TOO LARGE"};
    entry = format<EntryCapacity>("%.12G", number == 0 ? 0.0 : number);
}

void Calculator3D::digit(char character) {
    if (completed) clear();
    if (awaitingOperand) {
        entry = "0";
        awaitingOperand = false;
    }

    const auto text = Display();
    if (std::ranges::count_if(text, [](char c) { return c >= '0' && c <= '9'; }) >= 12) throw Calculator3DError{"LIMIT 12 DIGITS"};
    if (text == "0") entry[0] = character;
    else if (text == "-0") entry[1] = character;
    else {
        if (text.size() + 1 >= EntryCapacity) throw Calculator3DError{"NUMBER TOO LARGE"};
        entry.push_back(character);
    }
}

void Calculator3D::updateExpression() {
    if (pending != 0) expression = format<TextCapacity>("%.12G %c", accumulator, pending);
    else expression.clear();
}

void Calculator3D::addHistory(double left, char operation, double right) {
    auto text = format<TextCapacity>("%.12G %c %.12G = %s", left, operation, right, entry.c_str());
    if (history.size() == HistoryCapacity) history.pop_back();
    history.insert(history.begin(), std::move(text));
}

void Calculator3D::calculate(double right, char operation) {
    const auto left = accumulator;
    double result = 0;
    switch (operation) {
    case '+': result = left + right; break;
    case '-': result = left - right; break;
    case '*': result = left * right; break;
    case '/':
        if (right == 0) throw Calculator3DError{"DIVISION BY ZERO"};
        result = left / right;
        break;
    default: throw Calculator3DError{"Invalid operation"};
    }

    setValue(result);
    accumulator = value();
    addHistory(left, operation, right);
}

void Calculator3D::operate(char operation) {
    if (pending != 0 && !awaitingOperand) calculate(value(), pending);
    else if (pending == 0) accumulator = value();
    pending = operation;
    awaitingOperand = true;
    completed = false;
    repeatedOperation = 0;
    updateExpression();
}

void Calculator3D::evaluate() {
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

void Calculator3D::Press(Calculator3DKey key) {
    if (error) clear();
    try {
        const auto index = static_cast<std::size_t>(key);
        if (index >= Actions.size()) throw Calculator3DError{"Invalid calculator3D key"};
        std::visit([this](const auto& action) {
            using TAction = std::remove_cvref_t<decltype(action)>;
            if constexpr (std::is_same_v<TAction, Digit>) digit(action.Value);
            else if constexpr (std::is_same_v<TAction, Operation>) operate(action.Value);
            else command(action);
        }, Actions[index]);
    } catch (const Calculator3DError& failure) {
        clear();
        error = true;
        entry = format<EntryCapacity>("%s", failure.Message);
    }
}

void Calculator3D::command(Calculator3DKey key) {
    switch (key) {
        case Calculator3DKey::Clear: clear(); break;
        case Calculator3DKey::Equals: evaluate(); break;
        case Calculator3DKey::Decimal:
            if (completed) clear();
            if (awaitingOperand) { entry = "0"; awaitingOperand = false; }
            if (Display().find('.') == std::string_view::npos && Display().find('E') == std::string_view::npos) {
                const auto length = Display().size();
                if (length + 1 >= EntryCapacity) throw Calculator3DError{"NUMBER TOO LARGE"};
                entry.push_back('.');
            }

            break;
        case Calculator3DKey::Backspace:
            if (completed || awaitingOperand) { entry = "0"; completed = false; awaitingOperand = false; }
            else if (Display().find('E') != std::string_view::npos) throw Calculator3DError{"PRESS AC TO CLEAR"};
            else {
                entry.pop_back();
                if (Display().empty() || Display() == "-") { entry = "0"; }
            }

            break;
        case Calculator3DKey::Sign:
            if (Display().front() == '-') entry.erase(0, 1);
            else {
                const auto length = Display().size();
                if (length + 1 >= EntryCapacity) throw Calculator3DError{"NUMBER TOO LARGE"};
                entry.insert(entry.begin(), '-');
            }

            awaitingOperand = false;
            if (completed) accumulator = value();
            break;
        case Calculator3DKey::Percent:
            setValue((pending == '+' || pending == '-') ? accumulator * value() / 100 : value() / 100);
            awaitingOperand = false;
            if (completed) accumulator = value();
            break;
        default: throw Calculator3DError{"Invalid calculator3D command"};
    }
}

