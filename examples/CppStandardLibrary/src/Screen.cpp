#include "Screen.hpp"
#include <algorithm>
#include <stdexcept>

namespace {

constexpr int Width = 1920;
constexpr int Height = 1080;
constexpr std::uint32_t Ink = 0xffe6f3f7;
constexpr std::uint32_t Mint = 0xff44dabe;
constexpr std::uint32_t Red = 0xffff8585;

std::string visible(std::string_view text) {
    std::string result;
    text = text.substr(0, 90);
    result.reserve(text.size());

    for (char value : text) {
        if (value >= 'a' && value <= 'z') value = static_cast<char>(value - 'a' + 'A');
        if ((value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9') || value == ' ' || value == '.' || value == ':' || value == '-' || value == '+' || value == '/') result.push_back(value);
        else result.push_back(' ');
    }

    return result;
}


}

Screen::Screen() : display(Width, Height, 2), canvas(Width, Height), renderer(Width, Height) {}

void Screen::Begin(std::size_t index, std::string_view name) {
    if (index != rows.size()) throw std::out_of_range("Test groups must begin in order");
    rows.push_back({visible(name), "RUNNING"});
    summary = "RUNNING CHECKS";
    draw();
}

void Screen::Finish(std::size_t index, bool passed, std::string_view message) {
    rows.at(index).Status = passed ? "PASS" : "FAIL";
    if (!passed) { failed = true; detail = visible(message); }
    draw();
}

void Screen::Complete(unsigned failures) {
    failed = failures != 0;
    summary = std::to_string(rows.size()) + " GROUPS / " + std::to_string(failures) + " FAILURES";
    draw();
}

void Screen::Error(std::string_view message) {
    failed = true;
    summary = "CHECK RUN FAILED";
    detail = visible(message);
    draw();
}

void Screen::draw() {
    canvas.Clear();
    canvas.Rectangle(0, 0, Width, Height, 0xff101c2a);
    canvas.Rectangle(120, 76, 1680, 4, Mint);
    canvas.Text(120, 112, "ANYDEVKIT C++ STANDARD LIBRARY", 5, Ink);
    canvas.Text(120, 176, summary, 3, failed ? Red : Mint);

    const auto rowStep = rows.empty() ? 42 : std::min(42, 630 / static_cast<int>(rows.size()));
    if (rowStep < 29) throw std::length_error("Too many test groups for the results screen");

    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto top = 245 + static_cast<int>(index) * rowStep;
        canvas.Rectangle(120, top - 7, 1680, rowStep - 8, index % 2 == 0 ? 0xff1b2a3b : 0xff172333);
        canvas.Text(144, top, rows[index].Name, 3, Ink);
        canvas.Text(1510, top, rows[index].Status, 3, rows[index].Status == "FAIL" ? Red : Mint);
    }

    canvas.Text(120, 908, detail, 3, Red);
    canvas.Text(120, 994, "CLOSE THIS APPLICATION FROM THE SYSTEM MENU", 3, 0xff9caec2);
    renderer.Draw(display, canvas.Commands());
}

void Screen::KeepOpen() { for (;;) display.Wait(); }

