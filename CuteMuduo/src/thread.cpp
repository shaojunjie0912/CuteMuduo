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
    started_ = true;
    sem_t sem;  // 信号量(等子线程 tid_ 被成功赋值)
    sem_init(&sem, false, 0);
    thread_ = std::make_shared<std::thread>([&] {
        tid_ = current_thread::Tid();  // 更新线程 ID
        sem_post(&sem);                // 通知主线程 tid_ 已经被赋值
        func_();
    });
    // 这里必须等待获取上面新创建的线程的 tid_ 值
    sem_wait(&sem);
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
