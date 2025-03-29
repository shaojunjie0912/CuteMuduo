#pragma once

#include <cutemuduo/coro/task.hpp>
#include <cutemuduo/coro/tcp_connection.hpp>
#include <cutemuduo/tcp_server.hpp>
#include <functional>
#include <memory>

namespace cutemuduo::coro {

class TcpServerCoro {
public:
    using ConnectionHandler = std::function<Task<void>(TcpConnectionCoro)>;

    explicit TcpServerCoro(TcpServer* server) : server_(server) {}

    // 设置协程风格的连接处理函数
    void SetConnectionHandler(ConnectionHandler handler) {
        handler_ = std::move(handler);

        server_->SetConnectionCallback([this](const TcpConnectionPtr& conn) {
            // 只处理新连接
            if (conn->IsConnected() && handler_) {
                // 启动协程处理连接
                auto coro_conn = MakeCoroConnection(conn);
                handler_(coro_conn);
            }
        });
    }

private:
    TcpServer* server_;
    ConnectionHandler handler_;
};

// 帮助函数，创建协程版TcpServer
inline TcpServerCoro MakeCoroServer(TcpServer* server) {
    return TcpServerCoro(server);
}

}  // namespace cutemuduo::coro
