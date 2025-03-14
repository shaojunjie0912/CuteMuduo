#include <cutemuduo/tcp_connection.hpp>

namespace cutemuduo {

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        printf("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
        exit(-1);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, std::string const& name_arg, int sockfd, InetAddress const& localAddr,
                             InetAddress const& peerAddr)
    : loop_() {}

TcpConnection::~TcpConnection() {}

void TcpConnection::SetState(StateE const& new_s) {
    state_ = new_s;
}

}  // namespace cutemuduo
