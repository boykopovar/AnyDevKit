#include "Interface.hpp"
#include "Settings.hpp"
#include "CalculatorError.hpp"
#include "prx/libc/include/Api.hpp"

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

Interface::Interface(int width, int height, Settings& settings) : settings(settings), width(width), height(height) {
    if (width < 1280 || height < 720) throw CalculatorError{"Calculator requires at least 1280 by 720"};
    state = settings.Load();
    calculator.RestoreHistory(state.History);
}

Interface::Bounds Interface::button(int index) const {
    const int gap = width / 100;
    const int left = width * 51 / 100;
    const int cellWidth = (width * 44 / 100 - gap * 3) / 4;
    const int cellHeight = (height * 72 / 100 - gap * 4) / 5;
    return {left + index % 4 * (cellWidth + gap), height * 16 / 100 + index / 4 * (cellHeight + gap), cellWidth, cellHeight};
}

void Interface::Navigate(int horizontal, int vertical) {
    if (selected >= 20) {
        const int index = selected - 20;
        if (editingSlider && horizontal != 0) {
            auto updated = state;
            updated.Vibration[index] = std::clamp(updated.Vibration[index] + horizontal * (index == 0 ? 5 : 1), 0, index == 0 ? 200 : 255);
            if (updated.Vibration != state.Vibration) {
                settings.Save(updated);
                state = updated;
            }
        }
        else if (horizontal > 0) selected = sliderReturn;
        if (vertical != 0) {
            selected = 20 + (index + vertical + 3) % 3;
            editingSlider = false;
        }
    } else if (horizontal < 0 && selected % 4 == 0) {
        sliderReturn = selected;
        selected = 20 + std::clamp(selected / 4 - 2, 0, 2);
        editingSlider = false;
    } else selected = (selected / 4 + vertical + 5) % 5 * 4 + (selected % 4 + horizontal + 4) % 4;
    dirty = true;
}

void Interface::Press(CalculatorKey key) {
    calculator.Press(key);
    const auto history = calculator.GetHistory();
    if (history != state.History) {
        auto updated = state;
        updated.History = history;
        settings.Save(updated);
        state = updated;
    }
    pressed = static_cast<int>(key);
    pressedFrames = 7;
    dirty = true;
}

void Interface::Activate() {
    if (selected < 20) Press(static_cast<CalculatorKey>(selected));
    else {
        editingSlider = !editingSlider;
        dirty = true;
    }
}

int Interface::VibrationDuration() const { return state.Vibration[0]; }
int Interface::LowFrequencyStrength() const { return state.Vibration[1]; }
int Interface::HighFrequencyStrength() const { return state.Vibration[2]; }
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
    if (calculator.HistoryCount() == 0) canvas.Text(margin + 30, height * 49 / 100, "STANDARD / 12 DIGITS", 2, Muted);
    for (std::size_t index = 0; index < calculator.HistoryCount(); ++index) {
        fitted(canvas, margin + 30, height * (46 + static_cast<int>(index) * 5) / 100, panelWidth - 60, calculator.History(index), 2, index == 0 ? Ink : Muted);
    }
    constexpr std::array<std::string_view, 3> sliderLabels{"VIBRATION DURATION", "LOW FREQUENCY", "HIGH FREQUENCY"};
    for (int index = 0; index < 3; ++index) {
        const int top = height * (60 + index * 10) / 100;
        const bool focused = selected == 20 + index;
        const int maximum = index == 0 ? 200 : 255;
        if (focused) canvas.RoundedRectangle(margin - 8, top - 8, panelWidth + 16, height * 8 / 100, 12, editingSlider ? 0xff356455 : 0xff294c58);
        canvas.Text(margin + 12, top + 4, sliderLabels[index], 2, focused ? Mint : Ink);
        char value[24];
        const int length = snprintf_nid_postfix(value, sizeof(value), index == 0 ? "%d MS" : "%d / 255", state.Vibration[index]);
        if (length < 0 || static_cast<std::size_t>(length) >= sizeof(value)) throw CalculatorError{"Vibration value formatting failed"};
        canvas.Text(margin + panelWidth - length * 12 - 12, top + 4, value, 2, Muted);
        const int trackX = margin + 20;
        const int trackWidth = panelWidth - 40;
        const int trackY = top + height * 4 / 100;
        const int fill = trackWidth * state.Vibration[index] / maximum;
        canvas.Rectangle(trackX, trackY, trackWidth, 6, 0xff3b5261);
        canvas.Rectangle(trackX, trackY, fill, 6, Mint);
        canvas.RoundedRectangle(trackX + fill - 6, trackY - 7, 12, 20, 6, focused ? Mint : Ink);
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
    canvas.Text(margin, height * 92 / 100, selected >= 20 ? (editingSlider ? "LEFT / RIGHT: ADJUST   X / ENTER: DONE   UP / DOWN: NEXT SLIDER" : "UP / DOWN: SLIDER   X / ENTER: EDIT   RIGHT: BACK TO KEYPAD") : "DPAD / ARROWS: MOVE   LEFT EDGE: SLIDERS   X / ENTER: SELECT", 2, Muted);
    canvas.Text(margin, height * 96 / 100, "TRIANGLE / I: CLEAR   SQUARE: EQUALS   F11: FULLSCREEN ON WINDOWS", 2, Muted);
    dirty = false;
}
