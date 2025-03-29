#include <cutemuduo/coro/task.hpp>
#include <cutemuduo/coro/tcp_server.hpp>
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/logger.hpp>
#include <cutemuduo/tcp_server.hpp>

using namespace cutemuduo;
using namespace cutemuduo::coro;

class EchoServerCoro {
public:
    EchoServerCoro(EventLoop* loop, InetAddress const& addr, std::string const& name)
        : server_(loop, addr, name), loop_(loop) {
        // 创建协程服务器包装器
        auto coro_server = MakeCoroServer(&server_);

        // 使用协程风格的连接处理函数
        coro_server.SetConnectionHandler([](TcpConnectionCoro conn) -> Task<void> {
            LOG_INFO("New connection established");

            try {
                while (true) {
                    // 等待客户端消息，直到遇到换行符
                    auto message = co_await conn.ReadUntil("\n");
                    LOG_INFO("Received: %s", message.c_str());

                    // 回复消息
                    co_await conn.AsyncSend(message + "\n");
                    LOG_INFO("Echo sent");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Connection error: %s", e.what());
            }
        });

        server_.SetThreadNum(4);
    }

    void Start() {
        server_.Start();
    }

private:
    TcpServer server_;
    EventLoop* loop_;
};

int main() {
    EventLoop loop;
    InetAddress addr{9012};
    EchoServerCoro server{&loop, addr, "CoroEchoServer"};
    server.Start();
    loop.Loop();
    return 0;
}
