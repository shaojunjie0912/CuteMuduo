#include <sys/eventfd.h>
//
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/logger.hpp>
#include <cutemuduo/poller.hpp>

namespace cutemuduo {

// TODO: 防止一个线程创建多个 EventLoop (原: __thread)
thread_local EventLoop* loop_in_this_thread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
constexpr int kPollTimeMs{10000};  // 10000 毫秒 = 10 秒钟

// 创建 wakeup_fd_
int CreateEventfd() {
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      poller_(Poller::NewDefaultPoller(this)),
      thread_id_(current_thread::Tid()),
      wakeup_fd_(CreateEventfd()),
      wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_)),
      calling_pending_functors_(false) {
    if (loop_in_this_thread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", loop_in_this_thread, thread_id_);
    } else {
        loop_in_this_thread = this;
    }
    // 添加 wakeup_channel_ 读感兴趣事件
    wakeup_channel_->EnableReading();  // NOTE: !!这里调用 Update 把 wakeup_channel_ 添加进 Poller
    // 注册 wakeup_channel_ 的读回调函数
    wakeup_channel_->SetReadCallback([this](Timestamp) { HandleRead(); });
}

EventLoop::~EventLoop() {
    // wakeup_channel_ 已经没用了, 清理一下
    wakeup_channel_->DisableAll();
    wakeup_channel_->Remove();
    close(wakeup_fd_);
    loop_in_this_thread = nullptr;
}

void EventLoop::Loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);
    while (!quit_) {
        active_channels_.clear();
        //          EventLoop
        //       ↙↗          ↘↖
        //    Poller        Channel
        poll_return_time_ = poller_->Poll(kPollTimeMs, &active_channels_);
        for (auto& channel : active_channels_) {
            channel->HandleEvent(poll_return_time_);  // 依次处理 channel 上的事件
        }
        DoPendingFunctors();  // TODO: mainloop -> subloop?
    }
    looping_ = false;
    LOG_INFO("EventLoop %p stop looping\n", this);
}

void EventLoop::Quit() {
    // 分两种情况: 1. 自己线程调用 Quit() 2. 其他线程调用 Quit() NOTE: 需要 Wakeup
    quit_ = true;
    // 如果当前线程 loop 没有任何事件发生, 则会阻塞在 while 中 epoller_->poll() 函数内
    // 因此需要另外一个线程唤醒当前线程(通过使当前线程内的 wakeup_channel_ 发生事件)
    if (!IsInLoopThread()) {
        Wakeup();
    }
}

bool EventLoop::IsInLoopThread() const {
    return thread_id_ == current_thread::Tid();
}

void EventLoop::RunInLoop(Functor cb) {
    // 如果当前线程是 EventLoop 所在线程, 直接执行 cb
    if (IsInLoopThread()) {
        cb();
    }
    // 如果当前线程不是 EventLoop 所在线程, 把 cb 放入队列中, 唤醒 EventLoop 所在线程执行 cb
    else {
        QueueInLoop(std::move(cb));
    }
}

void EventLoop::QueueInLoop(Functor cb) {
    {
        std::unique_lock lk{mtx_};
        pending_functors_.push_back(std::move(cb));
    }

    if (!IsInLoopThread() || calling_pending_functors_) {
        Wakeup();
    }
}

void EventLoop::DoPendingFunctors() {
    std::vector<Functor> functors;
    calling_pending_functors_ = true;  // 标记正在执行 pending_functors_ 中的回调函数
    // NOTE: 交换: 最小化锁的持有时间、避免死锁，提高并发性能
    {
        std::unique_lock lk{mtx_};
        functors.swap(pending_functors_);
    }
    // 依次执行 pending_functors_ 中的回调函数
    for (const auto& functor : functors) {
        functor();
    }
    calling_pending_functors_ = false;
}

void EventLoop::UpdateChannel(Channel* channel) {
    poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel) {
    poller_->RemoveChannel(channel);
}

bool EventLoop::HasChannel(Channel* channel) {
    return poller_->HasChannel(channel);
}

// NOTE: 复习一下 write & read 系统调用
// 向文件描述符 fd 写入 buf 中 count 个字节的数据
// ssize_t write(int fd, const void *buf, size_t count);
// 从文件描述符 fd 读取 count 个字节的数据到 buf
// ssize_t read(int fd, void *buf, size_t count);

void EventLoop::Wakeup() {
    // 向 wakeup_fd_ 写 8 bytes 数据
    // wakeup_channel_ 触发 kReadEvent 当前 loop 线程就会被唤醒
    uint64_t one = 1;  // 8 bytes
    ssize_t n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::Wakeup() writes %lu bytes instead of 8\n", n);
    }
}

void EventLoop::HandleRead() {
    // 当 wakeup_channel_ 由于被**写入**了数据而触发 kReadEvent 事件后
    // 自动调用回调函数来**读取** wakeup_fd_ 中的数据 NOTE: 生产-消费
    uint64_t one = 1;  // 8 bytes
    ssize_t n = read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::HandleRead() reads %lu bytes instead of 8\n", n);
    }
}

}  // namespace cutemuduo
