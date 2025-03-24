#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
//
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/socket.hpp>

namespace cutemuduo {

Socket::Socket(int sockfd) : sockfd_(sockfd) {}

Socket::~Socket() {
    close(sockfd_);
}

int Socket::sockfd() const {
    return sockfd_;
}

void Socket::BindAddress(InetAddress const& localaddr) {
    // NOTE: 将const sockaddr_in* 强制转换为 sockaddr*
    int ret = bind(sockfd_, (sockaddr*)localaddr.GetSockAddr(), sizeof(sockaddr_in));
    if (ret != 0) {
        printf("bind error\n");
    }
}

void Socket::Listen() {
    int ret = listen(sockfd_, 1024);
    if (ret != 0) {
        printf("listen error\n");
    }
}

int Socket::Accept(InetAddress* peeraddr) {  // NOTE: peeraddr: 对端地址
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    // NOTE: accept4() 与 accept() 的区别在于可以设置非阻塞和关闭连接时关闭文件描述符
    int connfd = accept4(sockfd_, reinterpret_cast<sockaddr*>(&addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peeraddr->SetSockAddr(addr);  // HACK: InetAddress.addr_ 只能通过 SetSockAddr() 设置
    }
    return connfd;
}

void Socket::ShutdownWrite() {
    if (shutdown(sockfd_, SHUT_WR) < 0) {
        printf("shutdown error\n");
    }
}

void Socket::SetTcpNoDelay(bool on) {
    // TCP_NODELAY 用于禁用 Nagle 算法
    // Nagle 算法用于减少网络上传输的小数据包数量
    // 将 TCP_NODELAY 设置为 1 可以禁用该算法, 允许小数据包立即发送
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

// 允许在TIME_WAIT 状态下的端口立即被重新绑定（bind）
// 解决服务器重启后端口无法立即复用的问题。
void Socket::SetReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

// 允许多个进程或线程在相同的 IP/端口上创建 socket 并监听
// 提高服务器的并发性能，常用于多线程服务器模型
void Socket::SetReusePort(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

// 开启或关闭 TCP 保活(Keep-Alive)机制,
// 即在长连接中检测对端是否存活, 防止“死连接”长期占用资源
void Socket::SetKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

}  // namespace cutemuduo
