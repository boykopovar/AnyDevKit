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
    for (char value : text.substr(0, 90)) {
        if (value >= 'a' && value <= 'z') value = static_cast<char>(value - 'a' + 'A');
        if ((value >= 'A' && value <= 'Z') || (value >= '0' && value <= '9') || value == ' ' || value == '.' || value == ':' || value == '-' || value == '+' || value == '/') result.push_back(value);
        else result.push_back(' ');
    }
    return result;
}

void render(std::span<std::uint32_t> pixels, std::span<const DrawCommand> commands) {
    std::fill(pixels.begin(), pixels.end(), 0xff101c2a);
    for (const auto& command : commands) {
        if (command.Parameters[0] != 0) throw std::invalid_argument("Text screen does not use rounded rectangles");
        std::uint32_t color = 0;
        constexpr std::array<unsigned, 4> shifts{16, 8, 0, 24};
        for (std::size_t channel = 0; channel < shifts.size(); ++channel) color |= static_cast<std::uint32_t>(command.Color[channel] * 255.0f + 0.5f) << shifts[channel];
        const auto x = static_cast<unsigned>(command.Rectangle[0]);
        const auto y = static_cast<unsigned>(command.Rectangle[1]);
        const auto width = static_cast<unsigned>(command.Rectangle[2]);
        const auto height = static_cast<unsigned>(command.Rectangle[3]);
        const auto scale = static_cast<unsigned>(command.Parameters[3]);
        for (unsigned row = 0; row < height; ++row) {
            for (unsigned column = 0; column < width; ++column) {
                if (scale != 0) {
                    const auto glyphRow = row / scale;
                    const auto bits = (command.Glyph[glyphRow / 4] >> ((glyphRow % 4) * 5)) & 31u;
                    if ((bits & (1u << (4 - column / scale))) == 0) continue;
                }
                const auto tileX = x + column;
                const auto tileY = y + row;
                const auto tile = static_cast<std::size_t>(tileY / 128) * ((Width + 127) / 128) + tileX / 128;
                const auto offset = ((tileY << 4) & 0x0070u) ^ ((tileY << 5) & 0x0f00u) ^ ((tileY << 9) & 0x1000u) ^ ((tileY << 8) & 0x4000u) ^ ((tileX << 2) & 0x000cu) ^ ((tileX << 5) & 0x0380u) ^ ((tileX << 4) & 0x0400u) ^ ((tileX << 6) & 0x0800u) ^ ((tileX << 9) & 0xa000u);
                pixels[tile * 16384 + offset / sizeof(std::uint32_t)] = color;
            }
        }
    }
}

}

Screen::Screen() : display(Width, Height), canvas(Width, Height) { draw(); }

void Screen::Begin(std::size_t index, std::string_view name) {
    rows.at(index) = {visible(name), "RUNNING"};
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
    canvas.Rectangle(120, 76, 1680, 4, Mint);
    canvas.Text(120, 112, "ANYDEVKIT C++ STANDARD LIBRARY", 5, Ink);
    canvas.Text(120, 176, summary, 3, failed ? Red : Mint);
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto top = 245 + static_cast<int>(index) * 42;
        canvas.Rectangle(120, top - 7, 1680, 34, index % 2 == 0 ? 0xff1b2a3b : 0xff172333);
        canvas.Text(144, top, rows[index].Name, 3, Ink);
        canvas.Text(1510, top, rows[index].Status, 3, rows[index].Status == "FAIL" ? Red : Mint);
    }
    canvas.Text(120, 908, detail, 3, Red);
    canvas.Text(120, 994, "CLOSE THIS APPLICATION FROM THE SYSTEM MENU", 3, 0xff9caec2);
    render(display.Acquire(), canvas.Commands());
    display.Present();
}

void Screen::KeepOpen() { for (;;) display.Wait(); }
