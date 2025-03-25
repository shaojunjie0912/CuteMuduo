#include <cutemuduo/event_loop_thread.hpp>
#include <cutemuduo/event_loop_thread_pool.hpp>

namespace cutemuduo {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, std::string const& name)
    : base_loop_(base_loop), name_(name), started_(false), num_threads_(0), next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::SetThreadNum(int num_threads) {
    num_threads_ = num_threads;
}

void EventLoopThreadPool::Start(ThreadInitCallback const& cb) {
    started_ = true;

    for (int i{0}; i < (int)num_threads_; ++i) {
        char* buf = new char[name_.size() + 32]{};
        snprintf(buf, name_.size() + 32, "%s%d", name_.c_str(), i);
        auto t{std::make_unique<EventLoopThread>(cb, buf)};  // NOTE: 智能指针
        loops_.push_back(t->StartLoop());                    // 底层创建线程绑定新 EventLoop 并返回 loop 地址
        threads_.push_back(std::move(t));                    // 移动 unique_ptr
        delete[] buf;
    }

    // HACK: 如果线程数为 0(其实这里不包括 base_loop_), 则直接在 base_loop 上执行回调
    if (num_threads_ == 0 && cb) {
        cb(base_loop_);
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
    auto loop{base_loop_};
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops() const {
    if (loops_.empty()) {
        return {base_loop_};
    } else {
        return loops_;
    }
}

bool EventLoopThreadPool::started() const {
    return started_;
}

std::string const& EventLoopThreadPool::name() const {
    return name_;
}

}  // namespace cutemuduo
