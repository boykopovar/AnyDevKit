#include "Interface.hpp"
#include "CalculatorError.hpp"

#include <algorithm>
#include <array>

namespace {

constexpr std::uint32_t Ink = 0xffedf4f8;
constexpr std::uint32_t Muted = 0xff879baa;
constexpr std::uint32_t Mint = 0xff56e0ba;
constexpr std::array<std::string_view, 20> Labels{"AC", "+/-", "%", "/", "7", "8", "9", "*", "4", "5", "6", "-", "1", "2", "3", "+", "DEL", "0", ".", "="};

void fitted(Canvas& canvas, int x, int y, int available, std::string_view text, int maximum, std::uint32_t color) {
    if (text.empty()) return;
    const int scale = std::min(maximum, available / static_cast<int>(text.size() * 6));
    if (scale < 1) throw CalculatorError{"Text exceeds layout"};
    canvas.Text(x, y, text, scale, color);
}

}

Interface::Interface(int width, int height) : width(width), height(height) {
    if (width < 1280 || height < 720) throw CalculatorError{"Calculator requires at least 1280 by 720"};
}

Interface::Bounds Interface::button(int index) const {
    const int gap = width / 100;
    const int left = width * 51 / 100;
    const int cellWidth = (width * 44 / 100 - gap * 3) / 4;
    const int cellHeight = (height * 72 / 100 - gap * 4) / 5;
    return {left + index % 4 * (cellWidth + gap), height * 16 / 100 + index / 4 * (cellHeight + gap), cellWidth, cellHeight};
}

int Interface::hit(int x, int y) const {
    for (int index = 0; index < 20; ++index) {
        const auto bounds = button(index);
        if (x >= bounds.x && y >= bounds.y && x < bounds.x + bounds.width && y < bounds.y + bounds.height) return index;
    }
    return -1;
}

void Interface::Navigate(int horizontal, int vertical) {
    selected = (selected / 4 + vertical + 5) % 5 * 4 + (selected % 4 + horizontal + 4) % 4;
    pointerVisible = false;
    dirty = true;
}

void Interface::Point(int x, int y) {
    if (pointerVisible && x == pointerX && y == pointerY) return;
    pointerX = std::clamp(x, 0, width - 20);
    pointerY = std::clamp(y, 0, height - 24);
    pointerVisible = true;
    const auto hovered = hit(pointerX, pointerY);
    if (hovered >= 0) selected = hovered;
    dirty = true;
}

void Interface::Press(CalculatorKey key) {
    calculator.Press(key);
    pressed = static_cast<int>(key);
    pressedFrames = 7;
    dirty = true;
}

void Interface::Activate() { Press(static_cast<CalculatorKey>(selected)); }
void Interface::Click() { if (hit(pointerX, pointerY) >= 0) Activate(); }
bool Interface::NeedsDraw() const { return dirty; }

void Interface::Tick() {
    if (pressedFrames > 0 && --pressedFrames == 0) { pressed = -1; dirty = true; }
}

void Interface::Draw(Canvas& canvas) {
    for (int y = 0; y < height; ++y) {
        const auto shade = static_cast<std::uint32_t>(y * 9 / height);
        canvas.Rectangle(0, y, width, 1, 0xff0b1420 + shade * 0x010101);
    }
    const int margin = width / 20;
    const int panelWidth = width * 40 / 100;
    canvas.RoundedRectangle(margin, height * 16 / 100, panelWidth, height * 40 / 100, 24, 0xff192a38);
    canvas.Rectangle(margin + 28, height * 16 / 100 + 30, 48, 4, Mint);
    canvas.Text(margin, height * 6 / 100, "ANYDEVKIT", 3, Mint);
    canvas.Text(margin + 210, height * 6 / 100 + 4, "/ CALCULATOR", 2, Muted);
    canvas.Text(width * 51 / 100, height * 9 / 100, "MAKE IT ADD UP.", 3, Ink);
    fitted(canvas, margin + 30, height * 23 / 100, panelWidth - 60, calculator.Expression(), 3, Muted);
    fitted(canvas, margin + 30, height * 34 / 100, panelWidth - 60, calculator.Display(), calculator.HasError() ? 4 : 14, calculator.HasError() ? 0xffffa99b : Ink);
    canvas.Text(margin + 30, height * 49 / 100, calculator.HasError() ? "PRESS ANY KEY TO CONTINUE" : "STANDARD / 12 DIGITS", 2, Muted);
    canvas.Text(margin, height * 62 / 100, "RECENT", 2, Mint);
    if (calculator.HistoryCount() == 0) canvas.Text(margin, height * 68 / 100, "YOUR CALCULATIONS APPEAR HERE", 2, Muted);
    for (std::size_t index = 0; index < calculator.HistoryCount(); ++index) {
        fitted(canvas, margin, height * 68 / 100 + static_cast<int>(index) * height / 23, panelWidth, calculator.History(index), 2, index == 0 ? Ink : Muted);
    }
    for (int index = 0; index < 20; ++index) {
        const auto bounds = button(index);
        const bool accent = index == 19;
        const bool focused = index == selected;
        auto color = accent ? Mint : index % 4 == 3 ? 0xff294c58 : index < 4 || index == 16 ? 0xff283744 : 0xff20303f;
        if (index == pressed) color = 0xff97f3d7;
        if (focused) canvas.RoundedRectangle(bounds.x - 4, bounds.y - 4, bounds.width + 8, bounds.height + 8, 22, Mint);
        canvas.RoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height, 18, color);
        const int scale = Labels[index].size() > 1 ? 4 : 6;
        canvas.Text(bounds.x + (bounds.width - static_cast<int>(Labels[index].size()) * 6 * scale + scale) / 2, bounds.y + (bounds.height - 7 * scale) / 2, Labels[index], scale, accent || index == pressed ? 0xff102b27 : Ink);
    }
    canvas.Text(margin, height * 95 / 100, "DPAD / STICK: MOVE   X: SELECT   O: DELETE   TRIANGLE: CLEAR", 2, Muted);
    if (pointerVisible) {
        for (int row = 0; row < 22; ++row) canvas.Rectangle(pointerX, pointerY + row, row * 3 / 4 + 2, 1, 0xff07111b);
        for (int row = 2; row < 18; ++row) canvas.Rectangle(pointerX + 2, pointerY + row, std::max(1, row * 3 / 4 - 2), 1, Ink);
    }
    dirty = false;
}
