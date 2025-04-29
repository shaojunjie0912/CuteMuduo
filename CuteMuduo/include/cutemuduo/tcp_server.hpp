#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
//
#include <cutemuduo/callbacks.hpp>

namespace cutemuduo {

class EventLoop;
class InetAddress;
class Acceptor;
class EventLoopThreadPool;

class TcpServer {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum class Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop, InetAddress const& listen_addr, std::string const& name,
              Option const& option = Option::kNoReusePort);

    ~TcpServer();

public:
    // NOTE: 以下用户自定义设置的回调会传入 TcpConnection 中的用户自定义回调
    // TcpConnection 中的 Channel 成员变量绑定的回调是 HandleRead, HandleWrite, HandleClose, HandleError
    // 只不过 HandleXXX 内部也会调用用户自定义的回调函数

    // **用户自定义设置** 线程初始化回调函数
    void SetThreadInitCallback(ThreadInitCallback cb);

    // **用户自定义设置** 连接建立后的回调函数
    void SetConnectionCallback(ConnectionCallback cb);

    // **用户自定义设置** 收到消息后的回调函数
    void SetMessageCallback(MessageCallback cb);

    // **用户自定义设置** 发送完消息后的回调函数
    void SetWriteCompleteCallback(WriteCompleteCallback cb);

public:
    // 设置底层 Subloop 个数(不包括 Baseloop(即 Mainloop))
    void SetThreadNum(int num_threads);

    // 启动服务器(开启监听)
    void Start();

private:
    void NewConnection(int sockfd, InetAddress const& peer_addr);

    void RemoveConnection(TcpConnectionPtr const& conn);

    void RemoveConnectionInLoop(TcpConnectionPtr const& conn);

public:
    std::string name() const { return name_; }

    std::string ip_port() const { return ip_port_; }

    EventLoop* loop() const { return loop_; }

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;  // **Mainloop** 用户自定义

    std::string ip_port_;  // 服务器 IP 地址和端口号
    std::string name_;     // 服务器名称

    std::unique_ptr<Acceptor> acceptor_;

    std::shared_ptr<EventLoopThreadPool> thread_pool_;  // 线程池

    // TODO: 为什么没有 close_callback_?

    ConnectionCallback connection_callback_;         // **用户自定义** 连接建立后的回调函数(传入 TcpConnection)
    MessageCallback message_callback_;               // **用户自定义** 收到消息后的回调函数(传入 TcpConnection)
    WriteCompleteCallback write_complete_callback_;  // **用户自定义** 发送完消息后的回调函数(传入 TcpConnection)

    ThreadInitCallback thread_init_callback_;  // **用户自定义** 线程初始化回调函数(默认为空)
    int num_threads_;                          // 线程数量(其实就是 Subloop 个数(不包括 Mainloop))
    std::atomic_int started_;                  // 服务器是否已经启动(用 int 判断防止 TcpServer **启动多次**)
    int next_conn_id_;                         // 下一个连接的 ID
    ConnectionMap connections_;                // 保存的所有连接
};

}  // namespace cutemuduo
