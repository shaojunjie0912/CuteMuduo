#include <sys/eventfd.h>
//
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/poller.hpp>

namespace cutemuduo {

// TODO: 防止一个线程创建多个EventLoop
// __thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
constexpr int kPollTimeMs{10000};  // 10000 毫秒 = 10 秒钟

// 创建 wakeupfd
int CreateEventfd() {
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        printf("Failed in eventfd\n");
        // LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      poller_(Poller::NewDefaultPoller(this)),
      thread_id_(current_thread::Tid()),
      wakeup_fd_(CreateEventfd()),
      wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_)) {
    // TODO:
    // if (t_loopInThisThread) {
    //     LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    // } else {
    //     t_loopInThisThread = this;
    // }
    wakeup_channel_->SetReadCallback();
}

EventLoop::~EventLoop() {}

void EventLoop::Loop() {
    looping_ = true;
    quit_ = false;

    printf("EventLoop::Loop(%p)\n", this);
    while (!quit_) {
        active_channels_.clear();
        //          EventLoop
        //       ↙↗          ↘↖
        //    Poller        Channel
        poll_return_time_ = poller_->Poll(kPollTimeMs, &active_channels_);
        for (auto& channel : active_channels_) {
            channel->HandleEvent(poll_return_time_);  // 依次处理 channel 上的事件
        }
        // TODO:
        // doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::Quit() {
    quit_ = true;
    // 如果当前线程 loop 没有任何事件发生
    // 则会阻塞在 while 中 epoller_->poll() 函数内
    // 因此需要另外一个线程唤醒当前线程(通过使当前线程内的 wakeup_fd_ 发生事件)
    if (!IsInLoopThread()) {
        Wakeup();
    }
}

bool EventLoop::IsInLoopThread() const {
    return thread_id_ == current_thread::Tid();
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

void EventLoop::Wakeup() {
    // 用来唤醒loop所在线程 向wakeupFd_写一个数据
    // wakeupChannel就发生读事件 当前loop线程就会被唤醒
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        printf("EventLoop::wakeup() writes %lu bytes instead of 8\n", n);
    }
}

}  // namespace cutemuduo
