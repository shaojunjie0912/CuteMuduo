#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/event_loop_thread.hpp>

namespace cutemuduo {

EventLoopThread::EventLoopThread(ThreadInitCallback const& cb, std::string const& name)
    : loop_(nullptr),
      exiting_(false),
      thread_([this] { ThreadFunc(); },
              name),  // NOTE: 用 ThreadFunc 初始化线程对象(还未执行)要等 StartLoop() 调用
      cb_(cb) {}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_) {
        loop_->Quit();
        thread_.Join();
    }
}

EventLoop* EventLoopThread::StartLoop() {
    // NOTE: 会创建一个新线程并执行 ThreadFunc()
    // ThreadFunc 中开启事件循环
    thread_.Start();
    EventLoop* loop = nullptr;
    {
        std::unique_lock lk{mtx_};
        // 等待 loop_ 在 ThreadFunc() 中被赋值局部 loop 的地址然后唤醒
        cv_.wait(lk, [this] { return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;  // 返回 loop 地址
}

void EventLoopThread::ThreadFunc() {
    EventLoop loop;  // NOTE: 创建局部 loop, 也即 one loop per thread 中的 loop
    if (cb_) {
        cb_(&loop);  // 执行用户自定义线程初始化回调函数
    }
    {
        std::unique_lock lk{mtx_};
        loop_ = &loop;
        cv_.notify_one();
    }
    loop.Loop();  // NOTE: 开始事件循环, 线程会阻塞在这里(局部 loop 生命周期一直存在)
    std::unique_lock lk{mtx_};
    loop_ = nullptr;
}

}  // namespace cutemuduo
