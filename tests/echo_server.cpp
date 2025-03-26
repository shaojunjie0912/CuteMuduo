#include <string>
//
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/logger.hpp>
#include <cutemuduo/tcp_connection.hpp>
#include <cutemuduo/tcp_server.hpp>

using namespace cutemuduo;

class EchoServer {
public:
    EchoServer(EventLoop* loop, InetAddress const& addr, std::string const& name)
        : server_(loop, addr, name), loop_(loop) {
        // 用户自定义连接建立后回调函数
        server_.SetConnectionCallback(
            [this](TcpConnectionPtr const& conn_ptr) { UserDefineConnectionCallback(conn_ptr); });

        // 用户自定义收到消息后回调函数
        server_.SetMessageCallback([this](TcpConnectionPtr const& conn_ptr, Buffer* buf, Timestamp) {
            UserDefineMessageCallback(conn_ptr, buf);
        });

        server_.SetThreadNum(4);  // 设置线程数
    }

    ~EchoServer() = default;

public:
    void Start() {
        server_.Start();
    }

private:
    // 用户自定义连接建立后回调函数
    void UserDefineConnectionCallback(TcpConnectionPtr const& conn_ptr) {
        // NOTE: 这里就对上了为什么连接断开也会调用 connection_callback_
        // 因为这里会加一个判断决定日志
        LOG_INFO("EchoServer - %s -> %s is %s", conn_ptr->GetPeerAddress().ToIpPort().c_str(),
                 conn_ptr->GetLocalAddress().ToIpPort().c_str(), conn_ptr->IsConnected() ? "UP" : "DOWN");
    }

    // 用户自定义收到消息后回调函数
    void UserDefineMessageCallback(TcpConnectionPtr const& conn_ptr, Buffer* buf) {
        auto msg{buf->RetrieveAllAsString()};  // 读出 buffer 中收到的所有数据
        conn_ptr->Send(msg);                   // echo 回声, 返回给客户端
        // conn_ptr->Shutdown();                  // 关闭连接
    }

private:
    TcpServer server_;
    EventLoop* loop_;
};

int main() {
    EventLoop loop;
    InetAddress addr{9012};
    EchoServer server{&loop, addr, "CuteEchoServer"};
    server.Start();
    loop.Loop();
    return 0;
}
