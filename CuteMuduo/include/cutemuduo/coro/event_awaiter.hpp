#pragma once

#include <coroutine>
#include <cutemuduo/event_loop.hpp>
#include <functional>

namespace cutemuduo::coro {

template <typename T>
class EventAwaiter {
public:
    EventAwaiter(EventLoop* loop, std::function<void(std::function<void(T)>)> operation)
        : loop_(loop), operation_(std::move(operation)) {}

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        operation_([this, handle](T result) mutable {
            result_ = std::move(result);
            // 在正确的EventLoop线程中恢复协程
            loop_->RunInLoop([handle]() mutable { handle.resume(); });
        });
    }

    T await_resume() noexcept {
        return std::move(result_);
    }

private:
    EventLoop* loop_;
    std::function<void(std::function<void(T)>)> operation_;
    T result_;
};

// 特化void类型
template <>
class EventAwaiter<void> {
public:
    EventAwaiter(EventLoop* loop, std::function<void(std::function<void()>)> operation)
        : loop_(loop), operation_(std::move(operation)) {}

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle) {
        operation_([this, handle]() mutable { loop_->RunInLoop([handle]() mutable { handle.resume(); }); });
    }

    void await_resume() noexcept {}

private:
    EventLoop* loop_;
    std::function<void(std::function<void()>)> operation_;
};

}  // namespace cutemuduo::coro
