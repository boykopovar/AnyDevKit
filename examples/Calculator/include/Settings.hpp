#ifndef ANYDEVKIT_CALCULATOR_SETTINGS_HPP
#define ANYDEVKIT_CALCULATOR_SETTINGS_HPP

#include <array>

class Settings final {
public:
    explicit Settings(int userId);
    ~Settings() noexcept(false);
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    std::array<int, 3> Load();
    void Save(const std::array<int, 3>& values);

private:
    int userId;
    int resource;
};

#endif
