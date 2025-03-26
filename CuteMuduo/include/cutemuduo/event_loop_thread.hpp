#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
//
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/thread.hpp>

namespace cutemuduo {

class EventLoop;

// EventLoop 和 Thread 的封装
class EventLoopThread : NonCopyable {
public:
    // 线程初始化回调函数类型
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(ThreadInitCallback const& cb = ThreadInitCallback(), std::string const& name = "");

    ~EventLoopThread();

    // 开启一个线程并内部调用 ThreadFunc 启动事件循环(返回 loop 地址)
    EventLoop* StartLoop();

private:
    // 线程实际执行函数
    void ThreadFunc();

private:
    EventLoop* loop_;             // 该线程所拥有的事件循环对象
    bool exiting_;                // 是否正在退出
    Thread thread_;               // 实际的线程对象
    std::mutex mtx_;              // 互斥体
    std::condition_variable cv_;  // 条件变量
    ThreadInitCallback cb_;       // **用户自定义?** 线程初始化回调函数
};

}  // namespace cutemuduo
