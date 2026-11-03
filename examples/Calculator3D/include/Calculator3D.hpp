#ifndef ANYDEVKIT_CALCULATOR3D_HPP
#define ANYDEVKIT_CALCULATOR3D_HPP

#include <array>
#include <cstddef>
#include <string_view>
#include <string>
#include <vector>

enum class Calculator3DKey { Clear, Sign, Percent, Divide, Seven, Eight, Nine, Multiply, Four, Five, Six, Subtract, One, Two, Three, Add, Backspace, Zero, Decimal, Equals };

struct Calculator3DHistory {
    std::array<std::array<char, 80>, 2> Entries{};
    std::size_t Count = 0;
    bool operator==(const Calculator3DHistory&) const = default;
};

class Calculator3D final {
public:
    Calculator3D();
    void Press(Calculator3DKey key);
    std::string_view Display() const;
    std::string_view Expression() const;
    std::string_view History(std::size_t index) const;
    std::size_t HistoryCount() const;
    bool HasError() const;
    Calculator3DHistory GetHistory() const;
    void RestoreHistory(const Calculator3DHistory& saved);

private:
    void clear();
    void command(Calculator3DKey key);
    void digit(char value);
    void operate(char operation);
    void evaluate();
    void calculate(double right, char operation);
    void setValue(double value);
    double value() const;
    void updateExpression();
    void addHistory(double left, char operation, double right);
    std::string entry = "0";
    std::string expression;
    std::vector<std::string> history;
    double accumulator = 0;
    double repeatedOperand = 0;
    char pending = 0;
    char repeatedOperation = 0;
    bool awaitingOperand = false;
    bool completed = false;
    bool error = false;
};

#endif
