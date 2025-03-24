#pragma once

#include <unistd.h>
//
#include <cutemuduo/poller.hpp>

namespace cutemuduo {

class Channel;

class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop* loop);

    ~EpollPoller();

public:
    // 内部就是调用epoll_wait，将有事件发生的channel通过activeChannels返回
    Timestamp Poll(int timeoutMs, ChannelList* active_channels) override;

    // 更新 channel 上感兴趣的事件(如果 Channel 不在 Poller 中则添加进 Poller)
    void UpdateChannel(Channel* channel) override;

    // 当连接销毁时, 从 EpollPoller 移除 channel (即epoll 中删除 fd)
    void RemoveChannel(Channel* channel) override;

private:
    // 更新 channel 通道，本质是调用了 epoll_ctl
    void Update(int operation, Channel* channel);

    // 把有事件发生的channel添加到activeChannels中
    void FillActiveChannels(int num_events, ChannelList* active_channels) const;

private:
    static const int kInitEventListSize = 16;  // epoll_wait 的初始事件列表大小
    int epoll_fd_;                             // epoll_create 创建的文件描述符(区别于其它fd)

    using EventList = std::vector<epoll_event>;  // epoll_event 的列表
    EventList events_;                           // epoll_wait 返回的事件列表
};

}  // namespace cutemuduo
