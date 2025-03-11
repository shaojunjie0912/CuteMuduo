#include <cutemuduo/epoll_poller.hpp>

namespace cutemuduo {

// cpp hpp ODR
Poller* Poller::NewDefaultPoller(EventLoop* loop) {
    return new EpollPoller(loop);
}
}  // namespace cutemuduo
