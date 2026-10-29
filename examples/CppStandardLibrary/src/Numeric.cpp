#include "Checks.hpp"
#include <chrono>
#include <cmath>
#include <complex>
#include <numbers>
#include <numeric>
#include <random>
#include <ratio>
#include <valarray>

void CheckNumeric() {
    Require(std::gcd(24, 18) == 6 && std::lcm(6, 8) == 24, "Integer mathematics");
    Require(std::sqrt(81.0) == 9.0 && std::abs(std::sin(std::numbers::pi)) < 1e-12, "Floating mathematics");
    std::complex<double> z(3, 4); Require(std::abs(z) == 5 && std::norm(z) == 25, "Complex arithmetic");
    std::valarray<int> values{1, 2, 3}; values *= 2; Require(values.sum() == 12, "Valarray");
    std::mt19937 generator(5489); Require(generator() == 3499211612u, "Deterministic generator");
    std::uniform_int_distribution<int> distribution(10, 20);
    for (int i = 0; i < 1000; ++i) { auto value = distribution(generator); Require(value >= 10 && value <= 20, "Random distribution"); }
    using namespace std::chrono;
    Require(duration_cast<milliseconds>(seconds(3)).count() == 3000, "Duration arithmetic");
    constexpr year_month_day date{year{2024}, month{2}, day{29}};
    static_assert(date.ok());
    Require(year_month_day{sys_days{date}} == date, "Calendar arithmetic");
    static_assert(std::ratio_equal_v<std::ratio<2, 4>, std::ratio<1, 2>>);
}
