#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

//
#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : NonCopyable {
public:
    // 线程初始化回调函数类型
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* base_loop, std::string const& name);

    ~EventLoopThreadPool();

public:
    // 设置线程数
    void SetThreadNum(int num_threads);

    // 启动线程池(开启 num_threads_ 个线程并在每个线程启动事件循环)
    void Start(ThreadInitCallback const& cb = ThreadInitCallback());

    EventLoop* GetNextLoop();

    std::vector<EventLoop*> GetAllLoops() const;

public:
    bool started() const;

    std::string const& name() const;

private:
    EventLoop* base_loop_;                                   // **主 EventLoop**
    std::string name_;                                       // 线程池名称
    bool started_;                                           // 是否启动
    int num_threads_;                                        // 线程数
    int next_;                                               // 下一个线程索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;  // 线程列表
    std::vector<EventLoop*> loops_;                          // EventLoop 列表
};

}  // namespace cutemuduo
