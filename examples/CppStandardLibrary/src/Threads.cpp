#include "Checks.hpp"

#include <atomic>
#include <barrier>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <latch>
#include <mutex>
#include <semaphore>
#include <thread>
#include <utility>
#include <vector>

void CheckThreads() {
    constexpr unsigned Workers = 8;
    constexpr unsigned Batches = 32;
    for (unsigned batch = 0; batch < Batches; ++batch) {
        std::vector<std::future<std::uint64_t>> results;
        std::vector<std::thread> threads;
        for (unsigned worker = 0; worker < Workers; ++worker) {
            std::promise<std::uint64_t> promise;
            results.push_back(promise.get_future());
            const auto expected = static_cast<std::uint64_t>(batch) * Workers + worker;
            threads.emplace_back([promise = std::move(promise), expected]() mutable { promise.set_value_at_thread_exit(expected); });
        }
        for (unsigned worker = 0; worker < Workers; ++worker) {
            auto moved = std::move(threads[worker]);
            Require(!threads[worker].joinable(), "Moving a thread retained ownership");
            if (worker == 0 && batch % 2 == 0) moved.detach();
            else moved.join();
            Require(moved.get_id() == std::thread::id{}, "Completing a thread retained its identity");
            Require(results[worker].get() == static_cast<std::uint64_t>(batch) * Workers + worker, "Thread exit future lost its value");
        }
    }

    std::atomic<unsigned> token{0};
    std::atomic<unsigned> completed{0};
    std::barrier start(Workers);
    std::vector<std::thread> threads;
    for (unsigned worker = 0; worker < Workers; ++worker) threads.emplace_back([&, worker] {
        start.arrive_and_wait();
        for (unsigned iteration = 0; iteration < 512; ++iteration) {
            auto observed = token.load(std::memory_order_acquire);
            while (observed != worker) {
                token.wait(observed, std::memory_order_acquire);
                observed = token.load(std::memory_order_acquire);
            }
            completed.fetch_add(1, std::memory_order_relaxed);
            token.store((worker + 1) % Workers, std::memory_order_release);
            token.notify_all();
        }
    });
    for (auto& thread : threads) thread.join();
    Require(completed == Workers * 512 && token == 0, "Atomic wait ring lost a wakeup");

    std::once_flag once;
    std::atomic<unsigned> initialized{0};
    std::atomic<bool> ready{false};
    std::atomic<bool> observed{true};
    threads.clear();
    for (unsigned worker = 0; worker < Workers; ++worker) threads.emplace_back([&] {
        for (unsigned attempt = 0; attempt < 1024; ++attempt) {
            std::call_once(once, [&] { ++initialized; std::this_thread::yield(); ready.store(true, std::memory_order_relaxed); });
            if (!ready.load(std::memory_order_relaxed)) observed.store(false);
        }
    });
    for (auto& thread : threads) thread.join();
    Require(initialized == 1 && observed, "call_once published partial initialization");

    std::mutex mutex;
    std::condition_variable_any condition;
    std::latch waiting(Workers);
    std::atomic<unsigned> cancelled{0};
    std::vector<std::jthread> cancellable;
    for (unsigned worker = 0; worker < Workers; ++worker) cancellable.emplace_back([&](std::stop_token stop) {
        std::unique_lock lock(mutex);
        waiting.count_down();
        if (!condition.wait(lock, stop, [] { return false; })) ++cancelled;
    });
    waiting.wait();
    cancellable.clear();
    Require(cancelled == Workers, "jthread destruction did not stop and join every waiter");

    std::counting_semaphore<Workers> semaphore(0);
    threads.clear();
    for (unsigned worker = 0; worker < Workers; ++worker) threads.emplace_back([&] { semaphore.release(); });
    for (unsigned worker = 0; worker < Workers; ++worker) semaphore.acquire();
    for (auto& thread : threads) thread.join();
}

