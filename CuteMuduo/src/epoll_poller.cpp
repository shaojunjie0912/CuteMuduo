#include <cutemuduo/epoll_poller.hpp>

namespace cutemuduo {

constexpr int kNew{-1};     // Channel 还没添加到 Poller 中
constexpr int kAdded{1};    // Channel 已经添加到 Poller 中
constexpr int kDeleted{2};  // Channel 已经从 Poller 中删除

// NOTE: 这里用 loop 作为形参构造基类 Poller
EpollPoller::EpollPoller(EventLoop* loop)
    :  // 基类绑定 EventLoop
      Poller(loop),
      // epoll_create1 相比于 epoll_create 删除了无用 size 且可以指定 flags
      epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
      // events_ 是 epoll_wait 返回的事件列表, 初始大小为 kInitEventListSize
      events_(kInitEventListSize) {}

EpollPoller::~EpollPoller() {  // 析构函数直接关闭 epoll 的文件描述符
    close(epoll_fd_);
}

Timestamp EpollPoller::Poll(int timeoutMs, ChannelList* active_channels) {
    int num_events = epoll_wait(epoll_fd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    Timestamp now(Timestamp::Now());

    if (num_events > 0) {
        // 将发生的事件填充到 active_channels 中, 以便 EventLoop 处理
        FillActiveChannels(num_events, active_channels);
        if (static_cast<size_t>(num_events) == events_.size()) {  // NOTE: 手动给 events_(vector) 扩容
            events_.resize(events_.size() * 2);
        }
    } else if (num_events == 0) {
        // printf("nothing happened\n");
    } else {
        if (errno != EINTR) {
            printf("epoll_wait() error:%d\n", errno);
        }
    }
    return now;
}

// channel: 修改了感兴趣事件后的 Channel 对象
void EpollPoller::UpdateChannel(Channel* channel) {
    int index{channel->index()};
    // 若 kNew 则额外更新哈希表映射 + 添加到 epoll 中
    if (index == kNew) {
        channels_[channel->fd()] = channel;  // NOTE: 哈希表 channels_ 来自基类 Poller
        channel->SetIndex(kAdded);           // 更新 Channel 的 index_
        Update(EPOLL_CTL_ADD, channel);      // 添加到 epoll 中
    }
    // 若 kDeleted 则添加到 epoll 中
    else if (index == kDeleted) {
        channel->SetIndex(kAdded);       // 更新 Channel 的 index_
        Update(EPOLL_CTL_ADD, channel);  // 添加到 epoll 中
    }
    // 若 kAdded 则更新 epoll 中的事件
    else {
        // 如果 channel <当前>没有事件, 则删除 epoll 中的 channel->fd
        if (channel->IsNoneEvent()) {
            channel->SetIndex(kDeleted);
            Update(EPOLL_CTL_DEL, channel);
        }
        // 如果 channel <当前>有事件, 则更新 epoll 中的 channel->fd 对应的事件
        else {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::RemoveChannel(Channel* channel) {
    channels_.erase(channel->fd());  // 从哈希表中删除
    if (channel->index() == kAdded) {
        Update(EPOLL_CTL_DEL, channel);  // 从 epoll 中删除 channel->fd
    }
    channel->SetIndex(-1);  // -1: kNew
}

// UpdateChannel/RemoveChannel 中涉及到 epoll_ctl 核心操作的封装
void EpollPoller::Update(int operation, Channel* channel) {
    epoll_event event;
    event.events = channel->events();  // channel 上(新的)感兴趣的事件
    event.data.fd = channel->fd();     // NOTE: event.data 是一个联合体
    event.data.ptr = channel;          // 作用: 通过 fd 找到对应的 Channel 对象

    if (epoll_ctl(epoll_fd_, operation, channel->fd(), &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            // LOG_ERROR << "epoll_ctl() del error:" << errno;
            printf("epoll_ctl() del error:%d\n", errno);
        } else {
            // LOG_FATAL << "epoll_ctl add/mod error:" << errno;
            printf("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

void EpollPoller::FillActiveChannels(int num_events, ChannelList* active_channels) const {
    for (int i{0}; i < num_events; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);  // 获取对应的 channel
        channel->SetRevents(events_[i].events);                         // 记录 channel 实际发生事件 revents_
        active_channels->push_back(channel);                            // 将 channel 添加到 active_channels 中
    }
}

}  // namespace cutemuduo
