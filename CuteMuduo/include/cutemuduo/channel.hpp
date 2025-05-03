#pragma once

#include <sys/epoll.h>

#include <functional>
#include <memory>
//
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class EventLoop;

// NOTE: Channel 不拥有 fd, 而 Socket 创建并拥有 fd, Socket 析构时关闭 fd

// HACK: 感觉 Channel 可以简单理解为对 fd 上发生事件及处理事件逻辑的封装
// active_channels: 有事件发生的 fd
class Channel : NonCopyable {
public:
    // 以 EventLoop (事件循环) 和 fd (文件描述符) 作为参数构造 Channel
    Channel(EventLoop* loop, int fd);

    // TODO: 什么也不做?
    ~Channel();

public:
    using EventCallback = std::function<void()>;

    // 读回调函数类型
    using ReadEventCallback = std::function<void(Timestamp)>;

    // 注册 Channel 的可读回调函数
    void SetReadCallback(ReadEventCallback cb);

    // 注册 Channel 的可写回调函数
    void SetWriteCallback(EventCallback cb);

    // 注册 Channel 的关闭回调函数
    void SetCloseCallback(EventCallback cb);

    // 注册 Channel 的错误回调函数
    void SetErrorCallback(EventCallback cb);

public:
    // 监听可读
    void EnableReading();

    // 取消监听可读
    void DisableReading();

    // 监听可写
    void EnableWriting();

    // 取消监听可写
    void DisableWriting();

    // 取消所有监听事件
    void DisableAll();

public:
    // 判断 fd 是否没有监听事件
    bool IsNoneEvent() const;

    // 判断 fd **当前** 是否对可写事件感兴趣
    bool IsWriting() const;

    // 判断 fd **当前** 是否有读事件
    bool IsReading() const;

    // 更新实际发生的事件
    void SetRevents(int revents);

    // 处理事件
    void HandleEvent(Timestamp receive_time);

    //
    void HandleEventWithGuard(Timestamp receiveTime);

public:
    int fd() const;

    int events() const;

    int index() const;

    void SetIndex(int index);

    void Tie(std::shared_ptr<void> const& obj);

    void Remove();

private:
    void Update();

private:
    EventLoop* loop_;  // 所属 EventLoop
    int fd_;           // 文件描述符
    int events_;       // 感兴趣的事件 (不同于实际发生)
    int revents_;      // 实际发生的事件
    int index_;        // Channel 在 Poller 中的状态(-1: 还没添加, 1: 已经添加, 2: 已经删除)

    // 无事件
    static const int kNoneEvent = 0;

    // NOTE: 补充
    // EPOLLIN:
    //      - 对于 TCP socket: 表示对端发送了数据或对端关闭了连接
    //      - 对于管道/文件/eventfd: 表示有数据可读
    // EPOLLPRI:
    //      - OOB 带外数据到达
    // EPOLLOUT:
    //      - 内核缓冲区可用, 可以发送数据

    // 可读事件
    // - socket 缓冲区有数据可读
    // - 对端关闭连接
    // - OOB 带外数据
    static const int kReadEvent = EPOLLIN | EPOLLPRI;

    // 可写事件
    // - socket 缓冲区有空间可写
    static const int kWriteEvent = EPOLLOUT;

    // 事件发生时对应的回调函数
    ReadEventCallback read_callback_;  // 绑定 TcpConnection::HandleRead(Timestamp receiveTime)
    EventCallback write_callback_;     // 绑定 TcpConnection::HandleWrite()
    EventCallback close_callback_;     // 绑定 TcpConnection::HandleClose()
    EventCallback error_callback_;     // 绑定 TcpConnection::HandleError()

    // HACK: 想象成 <监视> TcpConnection 的生命周期
    // 如果 lock() 失败, 说明 TcpConnection 对象已经销毁了, 就不调用回调
    std::weak_ptr<void> tie_;  // 绑定 shared_ptr<TcpConnection>
    bool tied_;
};
}  // namespace cutemuduo
