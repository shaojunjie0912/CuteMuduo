#pragma once

#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

class InetAddress;

class Socket : NonCopyable {
public:
    explicit Socket(int sockfd);

    ~Socket();

public:
    // bind
    void BindAddress(InetAddress const &localaddr);

    // listen
    void Listen();

    // accept
    int Accept(InetAddress *peeraddr);

    void ShutdownWrite();         // 关闭套接字的写端(即不能往外发送数据)(但读端可以接收数据)
    void SetTcpNoDelay(bool on);  // 设置Nagel算法
    void SetReuseAddr(bool on);   // 设置地址复用
    void SetReusePort(bool on);   // 设置端口复用
    void SetKeepAlive(bool on);   // 设置长连接

public:
    int sockfd() const;

private:
    int sockfd_;
};

}  // namespace cutemuduo
