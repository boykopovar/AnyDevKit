#ifndef ANYDEVKIT_CPP_STANDARD_LIBRARY_SCREEN_HPP
#define ANYDEVKIT_CPP_STANDARD_LIBRARY_SCREEN_HPP

#include "Canvas.hpp"
#include "Display.hpp"
#include <array>
#include <string>

class Screen final {
public:
    Screen();
    ~Screen() noexcept(false);
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;
    void Begin(std::size_t index, std::string_view name);
    void Finish(std::size_t index, bool passed, std::string_view detail);
    void Complete(unsigned failures);
    void Error(std::string_view message);
    [[noreturn]] void KeepOpen();

private:
    struct Row { std::string Name; std::string Status; };
    void draw();
    Display display;
    Canvas canvas;
    std::span<std::uint32_t> frame;
    std::array<Row, 15> rows;
    std::string summary = "STARTING CHECKS";
    std::string detail;
    bool failed = false;
};

#endif
