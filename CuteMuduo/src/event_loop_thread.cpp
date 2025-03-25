#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/event_loop_thread.hpp>

namespace cutemuduo {

EventLoopThread::EventLoopThread(ThreadInitCallback const& cb, std::string const& name)
    : loop_(nullptr),
      exiting_(false),
      thread_([this] { ThreadFunc(); }, name),  // NOTE: 用 ThreadFunc 初始化线程对象(还未执行)要等 StartLoop() 调用
      cb_(cb) {}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_) {
        loop_->Quit();
        thread_.Join();
    }
}

EventLoop* EventLoopThread::StartLoop() {
    thread_.Start();  // 会创建一个新线程并执行 ThreadFunc()
    EventLoop* loop = nullptr;
    {
        std::unique_lock lk{mtx_};
        cv_.wait(lk, [this] { return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::ThreadFunc() {
    EventLoop loop;
    if (cb_) {
        cb_(&loop);  // 执行用户自定义线程初始化回调函数
    }
    {
        std::unique_lock lk{mtx_};
        loop_ = &loop;
        cv_.notify_one();
    }
    loop.Loop();  // 开始事件循环, 线程会阻塞在这里
    std::unique_lock lk{mtx_};
    loop_ = nullptr;
}

}  // namespace cutemuduo
