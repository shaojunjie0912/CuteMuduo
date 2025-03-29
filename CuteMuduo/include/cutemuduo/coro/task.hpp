#pragma once

#include <concepts>
#include <coroutine>
#include <exception>
#include <utility>

namespace cutemuduo::coro {

template <typename T = void>
class Task {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        T value;
        std::exception_ptr exception = nullptr;

        Task get_return_object() noexcept {
            return Task{handle_type::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        template <typename U>
            requires std::convertible_to<U, T>
        void return_value(U&& value) noexcept {
            this->value = std::forward<U>(value);
        }

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    // Void specialization
    template <>
    struct promise_type<void> {
        std::exception_ptr exception = nullptr;

        Task get_return_object() noexcept {
            return Task{handle_type::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    Task() noexcept : coro_(nullptr) {}

    Task(handle_type h) noexcept : coro_(h) {}

    ~Task() {
        if (coro_) coro_.destroy();
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : coro_(other.coro_) {
        other.coro_ = nullptr;
    }

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (coro_) coro_.destroy();
            coro_ = other.coro_;
            other.coro_ = nullptr;
        }
        return *this;
    }

    T result() const {
        if (coro_.promise().exception) std::rethrow_exception(coro_.promise().exception);
        if constexpr (!std::is_void_v<T>) return coro_.promise().value;
    }

    bool done() const noexcept {
        return !coro_ || coro_.done();
    }

    void resume() {
        if (!done()) coro_.resume();
    }

private:
    handle_type coro_;
};

}  // namespace cutemuduo::coro
