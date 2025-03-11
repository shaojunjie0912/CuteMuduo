#pragma once

#include <unistd.h>
//
#include <cutemuduo/poller.hpp>

namespace cutemuduo {

class Channel;

class EpollPoller : public Poller {
public:
    // NOTE: 这里用 loop 作为形参构造基类 Poller
    // epoll_create1 相比于 epoll_create 删除了无用 size 且可以指定 flags
    // EPOLL_CLOEXEC: close-on-exec 标志, 进程执行 exec 系统调用时, 会关闭所有文件描述符
    EpollPoller(EventLoop* loop) : Poller(loop), epoll_fd_(epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) {}

    ~EpollPoller() {
        close(epoll_fd_);
    }

public:
    Timestamp Poll(int timeoutMs, ChannelList* active_channels) override {}

    void UpdateChannel(Channel* channel) override {}

    void RemoveChannel(Channel* channel) override {}

private:
private:
    static const int kInitEventListSize = 16;  // epoll_wait 的初始事件列表大小
    int epoll_fd_;                             // epoll_create 创建的文件描述符(区别于其它fd)
    std::vector<epoll_event> events_;          // epoll_wait 返回的事件列表
};

}  // namespace cutemuduo
