#include "Checks.hpp"
#include <coroutine>
#include <exception>
#include <utility>

namespace {
class Sequence {
public:
    struct promise_type {
        int Value = 0;
        Sequence get_return_object() { return Sequence(std::coroutine_handle<promise_type>::from_promise(*this)); }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int value) noexcept { Value = value; return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { throw; }
    };
    explicit Sequence(std::coroutine_handle<promise_type> handle) : handle(handle) {}
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;
    ~Sequence() { handle.destroy(); }
    bool Next() { handle.resume(); return !handle.done(); }
    int Value() const { return handle.promise().Value; }
private:
    std::coroutine_handle<promise_type> handle;
};
Sequence sequence() { for (int i = 0; i < 10; ++i) co_yield i; }
}

void CheckCoroutines() {
    auto values = sequence(); int sum = 0; int count = 0;
    while (values.Next()) { sum += values.Value(); ++count; }
    Require(count == 10 && sum == 45, "Coroutine suspension and frame lifetime");
}
