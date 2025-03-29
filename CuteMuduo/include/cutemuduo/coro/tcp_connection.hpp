#pragma once

#include <cutemuduo/coro/event_awaiter.hpp>
#include <cutemuduo/coro/task.hpp>
#include <cutemuduo/tcp_connection.hpp>
#include <memory>
#include <string>

namespace cutemuduo::coro {

class TcpConnectionCoro {
public:
    explicit TcpConnectionCoro(TcpConnectionPtr conn) : conn_(std::move(conn)) {}

    // 异步读取数据，返回可等待对象
    EventAwaiter<std::string> ReadUntil(const std::string& delim) {
        return EventAwaiter<std::string>(conn_->GetLoop(), [this, delim](auto callback) {
            // 保存临时状态，等待数据到达
            auto state = std::make_shared<ReadState>();
            state->delim = delim;
            state->callback = std::move(callback);
            state->conn = conn_;

            // 设置一次性消息回调
            conn_->SetMessageCallback([state](const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
                auto data = buf->RetrieveAllAsString();
                state->buffer += data;

                // 检查是否包含分隔符
                auto pos = state->buffer.find(state->delim);
                if (pos != std::string::npos) {
                    auto result = state->buffer.substr(0, pos);
                    state->buffer = state->buffer.substr(pos + state->delim.size());
                    // 恢复原始回调并通知协程
                    state->callback(result);
                }
            });
        });
    }

    // 异步发送数据，返回可等待对象
    EventAwaiter<void> AsyncSend(const std::string& message) {
        return EventAwaiter<void>(conn_->GetLoop(), [this, message](auto callback) {
            conn_->Send(message);
            conn_->SetWriteCompleteCallback([callback](const TcpConnectionPtr&) { callback(); });
        });
    }

private:
    struct ReadState {
        std::string buffer;
        std::string delim;
        std::function<void(std::string)> callback;
        TcpConnectionPtr conn;
    };

    TcpConnectionPtr conn_;
};

// 帮助函数，创建协程版TcpConnection
inline TcpConnectionCoro MakeCoroConnection(const TcpConnectionPtr& conn) {
    return TcpConnectionCoro(conn);
}

}  // namespace cutemuduo::coro
