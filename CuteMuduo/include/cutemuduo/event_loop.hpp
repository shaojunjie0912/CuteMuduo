#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
//
#include <cutemuduo/current_thread.hpp>
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class Channel;
class Poller;

class EventLoop : NonCopyable {
public:
    EventLoop();
    ~EventLoop();

public:
    // 开启事件循环
    void Loop();

    // 退出事件循环
    void Quit();

    // 通过 wakeup_fd_ 唤醒 EventLoop
    void Wakeup();

    // 判断当前 EventLoop 对象是否在自己的线程里
    bool IsInLoopThread() const;

    using Functor = std::function<void()>;

    // 在 EventLoop 当前所在线程中执行cb
    void RunInLoop(Functor cb);

    // 把上层注册的回调函数cb放入队列中, 唤醒loop所在的线程执行cb
    void QueueInLoop(Functor cb);

    // 在 EventLoop 所在线程中执行 pending_functors_ 中的回调函数
    void DoPendingFunctors();

public:
    // 以下均调用 poller 的方法
    void UpdateChannel(Channel* channel);
    void RemoveChannel(Channel* channel);
    bool HasChannel(Channel* channel);

private:
    // wakeup_channel_ 的读回调函数
    void HandleRead();

private:
    std::atomic_bool looping_;  // 标记当前 EventLoop 是否处于事件循环中
    std::atomic_bool quit_;
    std::mutex mtx_;

    Timestamp poll_return_time_;  // Poller返回发生事件的Channels的时间点
    std::unique_ptr<Poller> poller_;

    using ChannelList = std::vector<Channel*>;
    ChannelList active_channels_;  // 返回Poller检测到当前有事件发生的所有Channel列表

    // =================== one loop per thread ===================
    pid_t thread_id_;                          // 用于标识当前EventLoop所属的线程
    int wakeup_fd_;                            // 用于唤醒EventLoop的文件描述符
    std::unique_ptr<Channel> wakeup_channel_;  // 用于唤醒EventLoop的Channel

    std::vector<Functor> pending_functors_;      // 用于存放需要在EventLoop所在线程中执行的回调函数
    std::atomic_bool calling_pending_functors_;  // 标记当前是否正在执行 pending_functors_ 中的回调函数
};

}  // namespace cutemuduo
