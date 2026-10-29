#include "Checks.hpp"
#include <exception>
#include <string>
#include <system_error>
#include <typeinfo>

void CheckExceptions() {
    struct Observer { int& Count; ~Observer() { Count = std::uncaught_exceptions(); } };
    int count = 0; std::exception_ptr saved;
    try {
        Observer observer{count};
        throw std::runtime_error(std::string(128, 'x'));
    } catch (const std::runtime_error& error) {
        Require(std::string(error.what()) == std::string(128, 'x'), "Exception message");
        saved = std::current_exception();
    }
    Require(count == 1 && std::uncaught_exceptions() == 0 && saved, "Exception unwinding");
    auto copy = saved; saved = nullptr;
    ExpectException<std::runtime_error>([&] { std::rethrow_exception(copy); });
    auto made = std::make_exception_ptr(std::invalid_argument("created"));
    ExpectException<std::invalid_argument>([&] { std::rethrow_exception(made); });
    try {
        try { throw std::out_of_range("inner"); }
        catch (...) { std::throw_with_nested(std::runtime_error("outer")); }
    } catch (const std::runtime_error& error) {
        ExpectException<std::out_of_range>([&] { std::rethrow_if_nested(error); });
    }
    struct Base { virtual ~Base() = default; }; struct Derived : Base {};
    Derived derived; Base& base = derived;
    Require(dynamic_cast<Derived*>(&base) == &derived && typeid(base) == typeid(Derived), "Runtime type information");
    auto code = std::make_error_code(std::errc::invalid_argument);
    Require(code == std::errc::invalid_argument && !code.message().empty(), "System error categories");
}
