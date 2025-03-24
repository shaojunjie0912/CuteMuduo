#pragma once

#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

class InetAddress;

// Socket 类是对 fd 的封装, 拥有 fd, 并负责在析构时关闭 fd
class Socket : NonCopyable {
public:
    explicit Socket(int sockfd);

    ~Socket();

public:
    // sockfd 绑定端口和地址
    void BindAddress(InetAddress const &localaddr);

    // 监听
    void Listen();

    // 接收连接, 返回连接的 fd
    int Accept(InetAddress *peeraddr);

    // 关闭套接字的写端(即不能往外发送数据)(但读端可以接收数据)
    void ShutdownWrite();

    // 设置Nagel算法
    void SetTcpNoDelay(bool on);

    // 设置地址复用
    void SetReuseAddr(bool on);

    // 设置端口复用
    void SetReusePort(bool on);

    // 设置长连接
    void SetKeepAlive(bool on);

public:
    // 返回 sockfd_
    int sockfd() const;

private:
    int sockfd_;  // TODO: 服务器的监听套接字?
};

}  // namespace cutemuduo
