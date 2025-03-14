#pragma once

#include <sys/epoll.h>

#include <functional>
#include <memory>
//
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class EventLoop;

// HACK: 感觉 Channel 可以简单理解为对 fd 上发生事件及处理事件逻辑的封装
// 所以活跃的 Channel(即 active_channels) 指的就是有事件发生的 fd
// NOTE: Channel 不拥有 fd, 而 Socket 创建并拥有 fd, Socket 析构时关闭 fd
class Channel : NonCopyable {
public:
    // 以 EventLoop(事件循环) 和 fd(文件描述符) 作为参数构造 Channel
    Channel(EventLoop* loop, int fd);
    ~Channel();

public:
    using EventCallback = std::function<void()>;  // muduo仍使用typedef
    using ReadEventCallback = std::function<void(Timestamp)>;

    // 设置回调函数对象
    void SetReadCallback(ReadEventCallback cb);
    void SetWriteCallback(EventCallback cb);
    void SetCloseCallback(EventCallback cb);
    void SetErrorCallback(EventCallback cb);

    // 向 epoll 中注册、删除 fd 感兴趣的事件
    void EnableReading();
    void DisableReading();
    void EnableWriting();
    void DisableWriting();
    void DisableAll();

    // 检查 fd <当前>事件状态
    bool IsNoneEvent() const;
    bool IsWriting() const;
    bool IsReading() const;

    // 更新实际发生的事件
    void SetRevents(int revents);

    // TODO: 处理事件
    void HandleEvent(Timestamp receive_time);

public:
    int fd() const;
    int events() const;
    int index() const;
    void SetIndex(int index);

private:
    void Update();

private:
    EventLoop* loop_;  // 所属 EventLoop
    int fd_;           // 文件描述符
    int events_;       // 感兴趣的事件
    int revents_;      // 实际发生的事件

    static const int kNoneEvent = 0;                   // 无事件
    static const int kReadEvent = EPOLLIN | EPOLLPRI;  // 读事件
    static const int kWriteEvent = EPOLLOUT;           // 写事件

    // 事件发生时对应的回调函数
    ReadEventCallback read_callback_;  // 绑定的是TcpConnection::handleRead(Timestamp receiveTime)
    EventCallback write_callback_;     // 绑定的是TcpConnection::handleWrite()
    EventCallback close_callback_;     // 绑定的是TcpConnection::handleClose()
    EventCallback error_callback_;     // 绑定的是TcpConnection::handleError()

    int index_;  // Channel 在 Poller 中的状态(-1: 还没添加, 1: 已经添加, 2: 已经删除)
};
}  // namespace cutemuduo
