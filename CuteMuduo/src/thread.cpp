#include <future>
//
#include <cutemuduo/current_thread.hpp>
#include <cutemuduo/thread.hpp>

namespace cutemuduo {

void Thread::SetDefaultName() {
    int num = ++num_created_;
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread %d", num);
        name_ = buf;
    }
}

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name) {
    SetDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();  // NOTE: detach? 非阻塞?
    }
}

void Thread::Start() {
    // TODO: 这里需要再验证一下, 虽然 Claude Sonnet 说正确
    started_ = true;
    std::promise<void> p;
    auto f = p.get_future();
    // HACK: 注意 () 和 mutable
    thread_ = std::make_shared<std::thread>([this, p = std::move(p)]() mutable {
        tid_ = current_thread::Tid();  // 更新线程 ID
        p.set_value();                 // 通知主线程
        func_();
    });
    f.wait();  // 等待线程 ID 更新
}

void Thread::Join() {
    joined_ = true;
    thread_->join();
}

bool Thread::Started() const {
    return started_;
}

pid_t Thread::Tid() const {
    return tid_;
}

std::string const& Thread::Name() const {
    return name_;
}

}  // namespace cutemuduo
