#pragma once

#include <memory>
#include <string>
#include <unordered_map>
//
#include <cutemuduo/callbacks.hpp>

namespace cutemuduo {

class EventLoop;
class Acceptor;

class TcpServer {
public:
private:
    std::string name_;  // 服务器名称
    std::string ip_;    // 服务器 IP 地址
    uint16_t port_;     // 服务器端口号

    EventLoop* loop_;

    std::unique_ptr<Acceptor> acceptor_;

    ConnectionCallback connection_callback_;  // **用户自定义** 连接建立后的回调函数
    CloseCallback close_callback_;            // **用户自定义** 连接关闭后的回调函数
    MessageCallback message_callback_;        // **用户自定义** 收到消息后的回调函数

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    ConnectionMap connections_;  // 保存所有连接
};

}  // namespace cutemuduo
