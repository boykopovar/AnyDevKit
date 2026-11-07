#include "Checks.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

void CheckMutexes() {
    using namespace std::chrono_literals;
    constexpr unsigned Workers = 8;
    constexpr unsigned Iterations = 12000;
    std::mutex first;
    std::mutex second;
    std::array<unsigned, 2> counters{};
    std::latch start(Workers);
    std::vector<std::thread> threads;

    for (unsigned worker = 0; worker < Workers; ++worker) threads.emplace_back([&, worker] {
        start.arrive_and_wait();
        for (unsigned iteration = 0; iteration < Iterations; ++iteration) {
            if (worker % 2 == 0) { std::scoped_lock lock(first, second); ++counters[0]; ++counters[1]; }
            else { std::scoped_lock lock(second, first); ++counters[0]; ++counters[1]; }
        }
    });
    for (auto& thread : threads) thread.join();
    Require(counters[0] == Workers * Iterations && counters[0] == counters[1], "Concurrent scoped_lock lost updates");

    std::recursive_timed_mutex recursive;
    {
        std::lock_guard outer(recursive);
        for (unsigned depth = 0; depth < 1024; ++depth) {
            std::lock_guard nested(recursive);
            std::unique_lock extra(recursive, std::try_to_lock);
            Require(extra.owns_lock(), "Recursive ownership was lost");
        }
    }

    std::timed_mutex timed;
    timed.lock();
    std::vector<std::future<bool>> timeouts;
    for (unsigned worker = 0; worker < 4; ++worker) timeouts.push_back(std::async(std::launch::async, [&] {
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            if (timed.try_lock_for(1ms)) { timed.unlock(); return false; }
        }
        return true;
    }));
    bool excluded = true;
    for (auto& timeout : timeouts) excluded = timeout.get() && excluded;
    timed.unlock();
    Require(excluded, "Timed mutex admitted a competing owner");

    std::shared_mutex shared;
    std::array<unsigned, 32> values{};
    unsigned generation = 0;
    for (unsigned index = 0; index < values.size(); ++index) values[index] = index;
    std::atomic<bool> coherent{true};
    threads.clear();
    for (unsigned worker = 0; worker < Workers; ++worker) threads.emplace_back([&, worker] {
        for (unsigned iteration = 0; iteration < 2048; ++iteration) {
            if (worker % 4 == 0) {
                std::unique_lock lock(shared);
                ++generation;
                for (unsigned index = 0; index < values.size(); ++index) values[index] = generation ^ index;
            } else {
                std::shared_lock lock(shared);
                for (unsigned index = 0; index < values.size(); ++index) if (values[index] != (generation ^ index)) coherent.store(false);
            }
        }
    });
    for (auto& thread : threads) thread.join();
    Require(coherent && generation == 4096, "Shared mutex exposed a partial update");
}

