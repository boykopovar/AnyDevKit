#ifndef ANYDEVKIT_CALCULATOR_HPP
#define ANYDEVKIT_CALCULATOR_HPP

#include <array>
#include <cstddef>
#include <string_view>

enum class CalculatorKey { Clear, Sign, Percent, Divide, Seven, Eight, Nine, Multiply, Four, Five, Six, Subtract, One, Two, Three, Add, Backspace, Zero, Decimal, Equals };

struct CalculatorHistory {
    std::array<std::array<char, 80>, 2> Entries{};
    std::size_t Count = 0;
    bool operator==(const CalculatorHistory&) const = default;
};

class Calculator final {
public:
    void Press(CalculatorKey key);
    std::string_view Display() const;
    std::string_view Expression() const;
    std::string_view History(std::size_t index) const;
    std::size_t HistoryCount() const;
    bool HasError() const;
    CalculatorHistory GetHistory() const;
    void RestoreHistory(const CalculatorHistory& saved);

private:
    void clear();
    void command(CalculatorKey key);
    void digit(char value);
    void operate(char operation);
    void evaluate();
    void calculate(double right, char operation);
    void setValue(double value);
    double value() const;
    void updateExpression();
    void addHistory(double left, char operation, double right);
    std::array<char, 32> entry{'0', '\0'};
    std::array<char, 80> expression{};
    std::array<std::array<char, 80>, 2> history{};
    std::size_t historyCount = 0;
    double accumulator = 0;
    double repeatedOperand = 0;
    char pending = 0;
    char repeatedOperation = 0;
    bool awaitingOperand = false;
    bool completed = false;
    bool error = false;
};

#endif
