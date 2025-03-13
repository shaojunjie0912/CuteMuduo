#pragma once

#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

class InetAddress;

class Socket : NonCopyable {
public:
    explicit Socket(int sockfd);

    ~Socket();

    int sockfd() const;

    // bind
    void BindAddress(const InetAddress &localaddr);

    // listen
    void Listen();

    // accept
    int Accept(InetAddress *peeraddr);

    // 设置半关闭
    void ShutdownWrite();
    void SetTcpNoDelay(bool on);  // 设置Nagel算法
    void SetReuseAddr(bool on);   // 设置地址复用
    void SetReusePort(bool on);   // 设置端口复用
    void SetKeepAlive(bool on);   // 设置长连接

private:
    int sockfd_;
};

}  // namespace cutemuduo
