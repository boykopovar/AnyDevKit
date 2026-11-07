#ifndef ANYDEVKIT_CALCULATOR3D_INTERFACE_HPP
#define ANYDEVKIT_CALCULATOR3D_INTERFACE_HPP

#include "Calculator3D.hpp"
#include "Canvas.hpp"
#include "Settings.hpp"
#include <array>

class Interface final {
public:
    Interface(int width, int height, Settings& settings);
    void Navigate(int horizontal, int vertical);
    void Activate();
    void Press(Calculator3DKey key);
    void Tick();
    int VibrationDuration() const;
    int LowFrequencyStrength() const;
    int HighFrequencyStrength() const;
    void Draw(Canvas& canvas);

private:
    struct Bounds { int x; int y; int width; int height; };
    Bounds button(int index) const;
    Settings& settings;
    Calculator3D calculator3D;
    Settings::Data state{};
    int width;
    int height;
    int selected = 17;
    int sliderReturn = 16;
    bool editingSlider = false;
    int pressed = -1;
    int pressedFrames = 0;
};

#endif

