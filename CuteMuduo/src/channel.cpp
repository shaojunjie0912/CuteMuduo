#include <cutemuduo/channel.hpp>
#include <cutemuduo/event_loop.hpp>

namespace cutemuduo {

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1) {}

Channel::~Channel() {}

// 设置回调函数对象
void Channel::SetReadCallback(ReadEventCallback cb) {
    read_callback_ = std::move(cb);
}
void Channel::SetWriteCallback(EventCallback cb) {
    write_callback_ = std::move(cb);
}
void Channel::SetCloseCallback(EventCallback cb) {
    close_callback_ = std::move(cb);
}
void Channel::SetErrorCallback(EventCallback cb) {
    error_callback_ = std::move(cb);
}

//               Channel Update/Remove
//                        ↓
// <"中转通道"> EventLoop UpdateChannel/RemoveChannel
//                        ↓
//              Poller updateChannel/RemoveChannel
void Channel::Update() {
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->UpdateChannel(this);
}

// 向 epoll 中注册、删除 fd 感兴趣的事件
// NOTE: 修改后要更新, Update() 本质调用 epoll_ctl()
void Channel::EnableReading() {
    events_ |= kReadEvent;
    Update();
}

void Channel::DisableReading() {
    events_ &= ~kReadEvent;
    Update();
}

void Channel::EnableWriting() {
    events_ |= kWriteEvent;
    Update();
}

void Channel::DisableWriting() {
    events_ &= ~kWriteEvent;
    Update();
}

void Channel::DisableAll() {
    events_ = kNoneEvent;
    Update();
}

// 判断 fd 是否没有事件
bool Channel::IsNoneEvent() const {
    return events_ == kNoneEvent;
}

// 判断 fd 是否有写事件
bool Channel::IsWriting() const {
    return events_ & kWriteEvent;
}

// 判断 fd 是否有读事件
bool Channel::IsReading() const {
    return events_ & kReadEvent;
}

// 更新实际发生的事件
void Channel::SetRevents(int revents) {
    revents_ = revents;
}

// 处理事件
// void Channel::HandleEvent(Timestamp receive_time) {}

int Channel::fd() const {
    return fd_;
}

int Channel::events() const {
    return events_;
}

int Channel::index() const {
    return index_;
}

void Channel::SetIndex(int index) {
    index_ = index;
}

}  // namespace cutemuduo
