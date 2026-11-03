#ifndef ANYDEVKIT_CALCULATOR3D_SETTINGS_HPP
#define ANYDEVKIT_CALCULATOR3D_SETTINGS_HPP

#include "Calculator3D.hpp"
#include <array>

class Settings final {
public:
    struct Data {
        std::array<int, 3> Vibration{55, 1, 1};
        Calculator3DHistory History{};
    };
    explicit Settings(int userId);
    ~Settings() noexcept(false);
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    Data Load();
    void Save(const Data& values);

private:
    int userId;
    int resource;
};

#endif
