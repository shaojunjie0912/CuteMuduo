#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
//
#include <cutemuduo/acceptor.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/logger.hpp>

namespace cutemuduo {

static int CreateNonblocking() {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, InetAddress const& listen_addr, bool reuse_port)
    : loop_(loop),
      accept_socket_(CreateNonblocking()),
      accept_channel_(loop, accept_socket_.sockfd()),
      listenning_(false) {
    accept_socket_.SetReuseAddr(true);        // 地址复用
    accept_socket_.SetReusePort(reuse_port);  // 端口复用
    accept_socket_.BindAddress(listen_addr);  // 绑定端口和地址

    accept_channel_.SetReadCallback([this](Timestamp) { HandleRead(); });  // 注册读回调 EPOLLIN
}

Acceptor::~Acceptor() {
    accept_channel_.DisableAll();
    accept_channel_.Remove();
}

bool Acceptor::listening() const {
    return listenning_;
}

void Acceptor::Listen() {
    listenning_ = true;
    accept_socket_.Listen();          // accept_socket_ 开启监听
    accept_channel_.EnableReading();  // accept_channel_ 添加读感兴趣事件
}

void Acceptor::SetNewConnectionCallback(NewConnectionCallback cb) {
    new_connection_callback_ = std::move(cb);
}

void Acceptor::HandleRead() {
    // 当有新连接到来时, 调用 Accept 接受连接
    InetAddress peer_addr;  // NOTE: 默认 port:0 ip:127.0.0.1
    int connfd = accept_socket_.Accept(&peer_addr);
    if (connfd >= 0) {
        // 如果有新连接回调函数则调用
        // NOTE: 由 TcpServer 通过 Acceptor::SetNewConnectionCallback 设置
        if (new_connection_callback_) {
            // NOTE: 将 connfd 和 peer_addr 传递给 TcpServer::NewConnection
            new_connection_callback_(connfd, peer_addr);
        }
        // 否则关闭连接
        else {
            close(connfd);
        }
    } else {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE) {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}

}  // namespace cutemuduo
