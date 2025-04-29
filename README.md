# CuteMuduo

CuteMuduo 是一个现代化的 C++ 网络库，是对陈硕老师的 [muduo](https://github.com/chenshuo/muduo) 高性能网络库的重新实现。本项目保留了 muduo 的核心设计理念，同时采用了 C++11/14/17/20 的现代特性，代码更加简洁、易读。

## 特性

- 基于 Reactor 模式的非阻塞 IO 网络库
- 使用 C++20 标准，利用智能指针、lambda 表达式等现代 C++ 特性
- one loop per thread 的线程模型
- 基于事件驱动的高效 IO 复用，当前使用 epoll
- 优雅的断开连接方式
- 基于 xmake 构建系统，简单易用

## 构建与运行

### 依赖

- 支持 C++20 的编译器（如 GCC 10+ 或 Clang 10+）
- xmake 2.9.8+（构建工具）
- Linux 系统环境

### 编译

```bash
# 克隆仓库
git clone https://github.com/shaojunjie0912/CuteMuduo.git
cd CuteMuduo

# 使用 xmake 构建
xmake
```

### 运行示例

```bash
# 运行 Echo 服务器
xmake run echo_server

# 在另一个终端运行 Echo 客户端
xmake run echo_client
```

## 核心组件

### 事件循环

- `EventLoop`: 事件循环的核心，包含 IO 复用和定时器
- `Channel`: 对文件描述符及其事件的封装
- `Poller`: IO 复用的抽象基类，当前实现为 `EpollPoller`

### 网络部分

- `TcpServer`: TCP 服务器抽象
- `TcpConnection`: 对 TCP 连接的抽象
- `Acceptor`: 接受新连接
- `Buffer`: 高效的缓冲区实现
- `InetAddress`: 对 sockaddr_in 的封装

### 多线程支持

- `EventLoopThread`: 运行事件循环的线程
- `EventLoopThreadPool`: 线程池，用于多线程 Reactor 模式

## 使用示例

### Echo 服务器示例

```cpp
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/tcp_server.hpp>

using namespace cutemuduo;

class EchoServer {
public:
    EchoServer(EventLoop* loop, InetAddress const& addr, std::string const& name)
        : server_(loop, addr, name), loop_(loop) {
        // 设置连接回调
        server_.SetConnectionCallback(
            [this](TcpConnectionPtr const& conn) {
                // 处理连接建立或断开
            });

        // 设置消息回调
        server_.SetMessageCallback(
            [this](TcpConnectionPtr const& conn, Buffer* buf, Timestamp) {
                // 读取并回显消息
                std::string msg = buf->RetrieveAllAsString();
                conn->Send(msg);
            });

        // 设置线程数
        server_.SetThreadNum(4);
    }

    void Start() { server_.Start(); }

private:
    TcpServer server_;
    EventLoop* loop_;
};

int main() {
    EventLoop loop;
    InetAddress addr(9012);  // 监听端口
    EchoServer server(&loop, addr, "EchoServer");
    server.Start();
    loop.Loop();  // 启动事件循环
    return 0;
}
```

## 设计原理

CuteMuduo 采用了 one loop per thread 的 Reactor 模式：

1. **主 Reactor**（Main Reactor）: 
   - 运行在主线程中
   - 负责接受客户端连接请求
   - 将新连接分发给子 Reactor

2. **子 Reactor**（Sub Reactor）: 
   - 运行在工作线程中
   - 负责处理已连接客户端的数据收发
   - 每个子 Reactor 由一个 EventLoop 管理

3. **非阻塞 IO + 事件驱动**:
   - 使用 epoll 作为 IO 复用机制
   - 基于事件的回调机制处理 IO 事件

## Reactor 线程模型

CuteMuduo 支持三种 Reactor 线程模型：

1. **单 Reactor 单线程模型**：一个线程完成所有工作
   ```cpp
   server.SetThreadNum(0);  // 不启动额外的线程
   ```

2. **单 Reactor 多线程模型**：主线程负责接受连接和事件分发，线程池负责业务处理
   ```cpp
   // 需自行实现业务线程池
   ```

3. **多 Reactor 多线程模型**（默认）：主线程负责接受连接，多个线程各自运行一个 Reactor 处理 IO
   ```cpp
   server.SetThreadNum(4);  // 启动4个IO线程
   ```

## 致谢

本项目参考了陈硕老师的 [muduo](https://github.com/chenshuo/muduo) 网络库，感谢陈硕老师的优秀设计和开源贡献。

## 许可证

CuteMuduo 使用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

