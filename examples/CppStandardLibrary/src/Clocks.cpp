#include "Checks.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

void CheckClocks() {
    using namespace std::chrono_literals;
    static_assert(std::chrono::steady_clock::is_steady);
    std::atomic<bool> ordered{true};
    std::vector<std::thread> threads;

    for (unsigned worker = 0; worker < 8; ++worker) threads.emplace_back([&] {
        auto previous = std::chrono::steady_clock::now();
        for (unsigned iteration = 0; iteration < 4096; ++iteration) {
            const auto now = std::chrono::steady_clock::now();
            if (now < previous) ordered.store(false);
            previous = now;
            const auto system = std::chrono::system_clock::now();
            const auto seconds = std::chrono::system_clock::to_time_t(system);
            const auto restored = std::chrono::system_clock::from_time_t(seconds);
            if (restored > system || system - restored >= 1s) ordered.store(false);
            static_cast<void>(std::chrono::high_resolution_clock::now());
            if (iteration % 64 == 0) std::this_thread::yield();
        }
    });
    for (auto& thread : threads) thread.join();
    Require(ordered, "Clock monotonicity or time_t conversion failed");

    for (const auto delay : std::array{1ms, 2ms, 5ms, 10ms}) {
        const auto before = std::chrono::steady_clock::now();
        std::this_thread::sleep_until(before + delay);
        Require(std::chrono::steady_clock::now() - before >= delay, "sleep_until completed before its deadline");
    }
}

