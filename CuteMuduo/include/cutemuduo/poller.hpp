#pragma once

#include <unordered_map>
#include <vector>
//
#include <cutemuduo/channel.hpp>
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class EventLoop;

// 基类, IO 多路复用
class Poller {
public:
    using ChannelList = std::vector<Channel*>;  // Poller 保存多个 Channel

    Poller(EventLoop* loop) : owner_loop_(loop) {}
    virtual ~Poller() = default;

public:
    // IO 多路复用统一接口
    virtual Timestamp Poll(int timeout_ms, ChannelList* active_channels) = 0;
    virtual void UpdateChannel(Channel* channel) = 0;
    virtual void RemoveChannel(Channel* channel) = 0;

    // 检查 Channel 是否在 Poller 中
    bool HasChannel(Channel* channel) const {
        auto it = channels_.find(channel->fd());
        return it != channels_.end() && it->second == channel;
    }
    // 创建默认 Poller
    static Poller* NewDefaultPoller(EventLoop* loop);

    // TODO: protected?
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;  // (哈希表) fd -> Channel 映射

private:
    EventLoop* owner_loop_;  // Poller 所属的事件循环 EventLoop
};

}  // namespace cutemuduo
