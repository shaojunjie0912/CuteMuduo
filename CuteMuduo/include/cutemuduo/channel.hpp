#pragma once

#include <sys/epoll.h>

#include <functional>
#include <memory>
//
#include <cutemuduo/non_copyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class EventLoop;

class Channel : NonCopyable {
public:
    using EventCallback = std::function<void()>;  // muduo仍使用typedef
    using ReadEventCallback = std::function<void(Timestamp)>;

    // 设置回调函数对象
    void SetReadCallback(ReadEventCallback cb) {
        read_callback_ = std::move(cb);
    }
    void SetWriteCallback(EventCallback cb) {
        write_callback_ = std::move(cb);
    }
    void SetCloseCallback(EventCallback cb) {
        close_callback_ = std::move(cb);
    }
    void SetErrorCallback(EventCallback cb) {
        error_callback_ = std::move(cb);
    }

    // 修改 fd 关注的事件 Update()->epoll_ctl()
    void EnableReading() {
        events_ |= kReadEvent;
        Update();
    }

    void DisableReading() {
        events_ &= ~kReadEvent;
        Update();
    }

    void EnableWriting() {
        events_ |= kWriteEvent;
        Update();
    }

    void DisableWriting() {
        events_ &= ~kWriteEvent;
        Update();
    }

    void DisableAll() {
        events_ = kNoneEvent;
        Update();
    }

    // 检查 fd 当前关注的事件
    bool IsNoneEvent() const {
        return events_ == kNoneEvent;
    }

    bool IsWriting() const {
        return events_ & kWriteEvent;
    }

    bool IsReading() const {
        return events_ & kReadEvent;
    }

    // 更新实际发生的事件
    void SetRevents(int revents) {
        revents_ = revents;
    }

    // 处理事件
    // void HandleEvent(Timestamp receive_time) {}
public:
    int fd() const {
        return fd_;
    }

private:
    void Update();

private:
    int fd_;       // 文件描述符
    int events_;   // 关注的事件
    int revents_;  // 实际发生的事件

    static const int kNoneEvent = 0;                   // 无事件
    static const int kReadEvent = EPOLLIN | EPOLLPRI;  // 读事件
    static const int kWriteEvent;                      // 写事件

    // 事件的回调函数
    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;

    // bool tied_;
};
}  // namespace cutemuduo
