#pragma once

#include <cutemuduo/poller.hpp>

namespace cutemuduo {

class Channel;

class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller();

public:
    Timestamp Poll(int timeoutMs, ChannelList* activeChannels) override;
    void UpdateChannel(Channel* channel) override;
    void RemoveChannel(Channel* channel) override;

private:
    int epoll_fd_;  // epoll_create 创建的文件描述符(区别于其它fd)
};

}  // namespace cutemuduo
