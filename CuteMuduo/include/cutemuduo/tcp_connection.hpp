#pragma once

#include <atomic>
#include <memory>
#include <string>
//
#include <cutemuduo/buffer.hpp>
#include <cutemuduo/callbacks.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/noncopyable.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

class EventLoop;
class Channel;
class Socket;

class TcpConnection : NonCopyable, public std::enable_shared_from_this<TcpConnection> {
    void Test();

public:
    TcpConnection(EventLoop* loop, std::string const& name_arg, int sockfd, InetAddress const& localAddr,
                  InetAddress const& peerAddr);

    ~TcpConnection();

public:
    void HandleRead(Timestamp receive_time);

    void HandleWrite();

    void HandleClose();

    void HandleError();

public:
    void SetConnectionCallback(ConnectionCallback const& cb);

    void SetMessageCallback(MessageCallback const& cb);

    void SetWriteCompleteCallback(WriteCompleteCallback const& cb);

    void SetHighWaterMarkCallback(HighWaterMarkCallback const& cb, size_t high_water_mark);

    void SetCloseCallback(CloseCallback const& cb);

public:
    void Send(std::string const& msg);

public:
    // 当 TcpServer 接受到新连接时调用
    void ConnectEstablished();

    // 当 TcpServer 连接销毁时调用
    void ConnectDestroyed();

private:
    void SendInLoop(void const* data, size_t len);

    void ShutdownInLoop();

public:
    EventLoop* GetLoop() const;

    std::string const& GetName() const;

    InetAddress const& GetLocalAddress() const;

    InetAddress const& GetPeerAddress() const;

    bool IsConnected() const;

private:
    enum class StateE {
        kConnecting,     // 正在连接
        kConnected,      // 已连接
        kDisconnecting,  // 正在断开连接
        kDisconnected    // 已断开连接
    };

    std::string StateToString() const;

    void SetState(StateE const& new_s);

private:
    EventLoop* loop_;            // 所属 EventLoop
    std::string name_;           // 连接名称
    std::atomic<StateE> state_;  // 连接状态
    bool reading_;               // 是否正在监听读事件

    std::unique_ptr<Socket> socket_;    // fd 底层的封装(bind, listen, accept)
    std::unique_ptr<Channel> channel_;  // fd 对应的 Channel

    InetAddress local_addr_;  // 本服务器地址
    InetAddress peer_addr_;   // 对端地址

    // 用户自定义这些事件的处理函数, 传递给 TcpServer
    // TcpServer 创建 TcpConnection 对象时设置这些回调函数到 TcpConnection 中

    size_t high_water_mark_;                          // 高水位标记
    ConnectionCallback connection_callback_;          // 连接建立回调
    MessageCallback message_callback_;                // 消息到达回调
    WriteCompleteCallback write_complete_callback_;   // 消息发送完毕回调
    HighWaterMarkCallback high_water_mark_callback_;  // 高水位回调
    CloseCallback close_callback_;                    // 连接关闭回调

    Buffer input_buffer_;   // 输入缓冲区
    Buffer output_buffer_;  // 输出缓冲区
};

}  // namespace cutemuduo
