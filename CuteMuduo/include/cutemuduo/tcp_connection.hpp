#pragma once

#include <atomic>
#include <memory>
#include <string>
//
#include <cutemuduo/callbacks.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class EventLoop;
class Channel;
class Socket;

class TcpConnection : NonCopyable {
public:
    TcpConnection(EventLoop* loop, std::string const& name_arg, int sockfd, InetAddress const& localAddr,
                  InetAddress const& peerAddr);
    ~TcpConnection();

public:
    void SetConnectionCallback(ConnectionCallback const& cb);
    void SetMessageCallback(MessageCallback const& cb);
    void SetWriteCompleteCallback(WriteCompleteCallback const& cb);
    void SetHighWaterMarkCallback(HighWaterMarkCallback const& cb, size_t high_water_mark);
    void SetCloseCallback(CloseCallback const& cb);

private:
    enum class StateE {
        kConnecting,     // 正在连接
        kConnected,      // 已连接
        kDisconnecting,  // 正在断开连接
        kDisconnected    // 已断开连接
    };

    void SetState(StateE const& new_s);

private:
    EventLoop* loop_;                   // 所属 EventLoop
    std::atomic<StateE> state_;         // 连接状态
    std::unique_ptr<Socket> socket_;    // fd 底层的封装(bind, listen, accept)
    std::unique_ptr<Channel> channel_;  // fd 对应的 Channel
    InetAddress local_addr_;            // 本服务器地址
    InetAddress peer_addr_;             // 对端地址

    // 用户自定义这些事件的处理函数, 传递给 TcpServer
    // TcpServer 创建 TcpConnection 对象时设置这些回调函数到 TcpConnection 中

    ConnectionCallback connection_callback_;          // 连接建立回调
    MessageCallback message_callback_;                // 消息到达回调
    WriteCompleteCallback write_complete_callback_;   // 消息发送完毕回调
    HighWaterMarkCallback high_water_mark_callback_;  // 高水位回调
    CloseCallback close_callback_;                    // 连接关闭回调

    size_t high_water_mark_;  // 高水位标记
};

}  // namespace cutemuduo
