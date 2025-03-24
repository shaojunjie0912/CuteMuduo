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
public:
    TcpConnection(EventLoop* loop, std::string const& name_arg, int sockfd, InetAddress const& localAddr,
                  InetAddress const& peerAddr);

    ~TcpConnection();

private:
    // 绑定 Channel 的可读回调函数
    void HandleRead(Timestamp receive_time);

    // 绑定 Channel 的可写回调函数
    void HandleWrite();

    // 绑定 Channel 的关闭回调函数
    void HandleClose();

    // 绑定 Channel 的错误回调函数
    void HandleError();

public:
    // 设置用户自定义的 **连接建立后** 的回调函数(由上层 TcpServer 调用)
    void SetConnectionCallback(ConnectionCallback cb);

    // 设置用户自定义的 **连接关闭后** 的回调函数(由上层 TcpServer 调用)
    void SetCloseCallback(CloseCallback cb);

    // 设置用户自定义的 **收到消息后** 的回调函数(由上层 TcpServer 调用)
    void SetMessageCallback(MessageCallback cb);

    // 设置用户自定义的 **发送完消息后** 的回调函数(由上层 TcpServer 调用)
    void SetWriteCompleteCallback(WriteCompleteCallback cb);

    // 设置用户自定义的高水位回调函数(由上层 TcpServer 调用)
    void SetHighWaterMarkCallback(HighWaterMarkCallback cb, size_t high_water_mark);

public:
    // 向对端发送消息
    void Send(std::string const& msg);

    // 在当前连接所属的 EventLoop 线程中发送消息
    void SendInLoop(void const* data, size_t len);

public:
    // 当 TcpServer 接受到新连接时调用
    void ConnectEstablished();

    // 当 TcpServer 连接销毁时调用
    void ConnectDestroyed();

private:
    // 关闭连接
    void Shutdown();

    // 在 EventLoop 线程中关闭连接
    void ShutdownInLoop();

public:
    // 获取当前连接所属的 EventLoop
    EventLoop* GetLoop() const;

    // 获取当前连接的名称
    std::string const& GetName() const;

    // 获取当前连接的本地地址信息
    InetAddress const& GetLocalAddress() const;

    // 获取当前连接的对端地址信息
    InetAddress const& GetPeerAddress() const;

    // 判断当前连接是否已经建立
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
    EventLoop* loop_;            // 所属 **Sub** EventLoop
    std::string name_;           // 连接名称
    std::atomic<StateE> state_;  // 连接状态
    bool reading_;               // 是否正在监听读事件

    std::unique_ptr<Socket> socket_;    // 已经连接的 socketfd
    std::unique_ptr<Channel> channel_;  // socketfd 对应的 Channel

    InetAddress local_addr_;  // 服务器地址信息
    InetAddress peer_addr_;   // 客户端地址信息

    // 用户自定义这些事件的处理函数, 传递给 TcpServer
    // TcpServer 创建 TcpConnection 对象时设置这些回调函数到 TcpConnection 中

    ConnectionCallback connection_callback_;         // **用户自定义** 连接建立后的回调函数
    CloseCallback close_callback_;                   // **用户自定义** 连接关闭后的回调函数
    MessageCallback message_callback_;               // **用户自定义** 收到消息后的回调函数
    WriteCompleteCallback write_complete_callback_;  // **用户自定义** 发送完消息后的回调函数

    size_t high_water_mark_;                          // 高水位标记(对用户态缓冲区 output_buffer_ 的大小限制)
    HighWaterMarkCallback high_water_mark_callback_;  // 高水位回调函数

    Buffer input_buffer_;   // 该 TCP 连接对应的 **用户** 输入缓冲区
    Buffer output_buffer_;  // 该 TCP 连接对应的 **用户** 输出缓冲区
};

}  // namespace cutemuduo
