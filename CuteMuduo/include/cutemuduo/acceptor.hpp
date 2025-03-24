#pragma once

#include <cutemuduo/channel.hpp>
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/socket.hpp>

namespace cutemuduo {

class EventLoop;
class InetAddress;

class Acceptor : NonCopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, InetAddress const&)>;

    Acceptor(EventLoop* loop, InetAddress const& listenAddr, bool reuseport);

    ~Acceptor();

public:
    // 判断是否正在监听
    bool listening() const;

    // 监听本地端口
    void Listen();

    // 设置新连接的回调函数
    void SetNewConnectionCallback(NewConnectionCallback cb);

private:
    void HandleRead();

    EventLoop* loop_;                                // main loop
    Socket accept_socket_;                           // listen socket(专门接受新连接)
    Channel accept_channel_;                         // listen channel
    bool listenning_;                                // 是否正在监听
    NewConnectionCallback new_connection_callback_;  // 新连接回调函数
};

}  // namespace cutemuduo
