#ifndef ANYDEVKIT_CALCULATOR_INTERFACE_HPP
#define ANYDEVKIT_CALCULATOR_INTERFACE_HPP

#include "Calculator.hpp"
#include "Canvas.hpp"

class Interface final {
public:
    Interface(int width, int height);
    void Navigate(int horizontal, int vertical);
    void Point(int x, int y);
    void Activate();
    void Click();
    void Press(CalculatorKey key);
    void Tick();
    bool NeedsDraw() const;
    void Draw(Canvas& canvas);

private:
    struct Bounds { int x; int y; int width; int height; };
    Bounds button(int index) const;
    int hit(int x, int y) const;
    Calculator calculator;
    int width;
    int height;
    int selected = 17;
    int pointerX = 0;
    int pointerY = 0;
    int pressed = -1;
    int pressedFrames = 0;
    bool pointerVisible = false;
    bool dirty = true;
};

#endif
