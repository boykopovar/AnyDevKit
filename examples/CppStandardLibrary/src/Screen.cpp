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

constexpr auto ColumnOffsets = [] {
    std::array<std::size_t, Width> offsets{};
    for (unsigned x = 0; x < Width; ++x) {
        const auto swizzle = ((x << 2) & 0x000cu) ^ ((x << 5) & 0x0380u) ^ ((x << 4) & 0x0400u) ^ ((x << 6) & 0x0800u) ^ ((x << 9) & 0xa000u);
        offsets[x] = static_cast<std::size_t>(x / 128) * 16384 + swizzle / sizeof(std::uint32_t);
    }

    return offsets;
}();

void fillRectangle(std::span<std::uint32_t> pixels, unsigned x, unsigned y, unsigned width, unsigned height, std::uint32_t color) {
    for (unsigned row = y; row < y + height; ++row) {
        const auto base = static_cast<std::size_t>(row / 128) * ((Width + 127) / 128) * 16384;
        const auto swizzle = (((row << 4) & 0x0070u) ^ ((row << 5) & 0x0f00u) ^ ((row << 9) & 0x1000u) ^ ((row << 8) & 0x4000u)) / sizeof(std::uint32_t);

        for (unsigned column = x; column < x + width; ++column) pixels[base + (ColumnOffsets[column] ^ swizzle)] = color;
    }
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
        if (scale == 0) {
            fillRectangle(pixels, x, y, width, height, color);
            continue;
        }

        for (unsigned row = 0; row < 7; ++row) {
            const auto bits = (command.Glyph[row / 4] >> ((row % 4) * 5)) & 31u;
            for (unsigned column = 0; column < 5; ++column) {
                if ((bits & (1u << (4 - column))) != 0) fillRectangle(pixels, x + column * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

}

Screen::Screen() : display(Width, Height, 2), canvas(Width, Height) {}

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

