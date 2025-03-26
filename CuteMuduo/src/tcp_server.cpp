#include <cutemuduo/acceptor.hpp>
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/event_loop_thread_pool.hpp>
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/logger.hpp>
#include <cutemuduo/tcp_connection.hpp>
#include <cutemuduo/tcp_server.hpp>

namespace cutemuduo {

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, InetAddress const& listen_addr, std::string const& name, Option const& option)
    : loop_(CheckLoopNotNull(loop)),
      ip_port_(listen_addr.ToIpPort()),
      name_(name),
      acceptor_(std::make_unique<Acceptor>(loop, listen_addr, option == Option::kReusePort)),
      thread_pool_(std::make_shared<EventLoopThreadPool>(loop, name)),
      num_threads_(0),
      started_(false),
      next_conn_id_(1) {
    // 为 Acceptor 设置新连接回调函数
    // 有新连接时, Acceptor::HandleRead() 会执行 TcpServer::NewConnection() 同时传入 connfd 和 peer_addr
    acceptor_->SetNewConnectionCallback(
        [this](int connfd, InetAddress const& peer_addr) { NewConnection(connfd, peer_addr); });
}

TcpServer::~TcpServer() {
    // NOTE: 设计哲学: 在 TcpConnection 自己的 loop 中调用 ConnectDestroyed
    // 且要保持先移除连接, 再销毁连接的顺序
    for (auto& [name, conn_ptr] : connections_) {
        auto conn_ptr_tmp{conn_ptr};  // 引用计数 +1
        conn_ptr.reset();             // NOTE: 指针置空(但由于引用计数不为 0, 因此不会析构对象)
        // HACK: conn_ptr_tmp 是局部变量, 离开作用域会销毁, 必须按值捕获!
        // 否则调用 conn_ptr_tmp->ConnectDestroyed() 会导致 conn_ptr_tmp 为悬垂指针, 未定义行为
        conn_ptr_tmp->GetLoop()->RunInLoop([conn_ptr_tmp] { conn_ptr_tmp->ConnectDestroyed(); });
    }
}

void TcpServer::Start() {
    // HACK: +1 == 0? 防止 TcpServer 被启动多次
    if (started_.fetch_add(1) == 0) {
        thread_pool_->Start(thread_init_callback_);         // 启动线程池(其实是开启 num_threads_ 个 Subloop)
        loop_->RunInLoop([this] { acceptor_->Listen(); });  // NOTE: 当前就是 Mainloop, 只需要启动 Acceptor 的监听
    }
}

void TcpServer::NewConnection(int connfd, InetAddress const& peer_addr) {
    // 轮询选择一个 Subloop 管理新连接
    auto sub_loop{thread_pool_->GetNextLoop()};  // 获取管理新连接的 Subloop
    char buf[64]{};
    snprintf(buf, sizeof(buf), "-%s#%d", ip_port_.c_str(), next_conn_id_++);  // 新连接的名称
    auto conn_name{name_ + buf};
    LOG_INFO("TcpServer::NewConnection [%s] - new connection [%s] from %s\n", name_.c_str(), conn_name.c_str(),
             peer_addr.ToIpPort().c_str());
    sockaddr_in local{};
    socklen_t addrlen = sizeof(local);
    if (getsockname(connfd, reinterpret_cast<sockaddr*>(&local), &addrlen) < 0) {
        LOG_ERROR("getsockname error\n");
    }
    InetAddress local_addr{local};  // 获取本地地址信息(构造 InetAddress 对象)
    // 构造 TcpConnection 对象
    auto conn_ptr{std::make_shared<TcpConnection>(sub_loop, conn_name, connfd, local_addr, peer_addr)};
    connections_[conn_name] = conn_ptr;                            // 保存新连接
    conn_ptr->SetConnectionCallback(connection_callback_);         // 设置连接建立后的回调函数
    conn_ptr->SetMessageCallback(message_callback_);               // 设置收到消息后的回调函数
    conn_ptr->SetWriteCompleteCallback(write_complete_callback_);  // 设置发送完消息后的回调函数

    // NOTE: 这里连接关闭回调函数是 TcpServer::RemoveConnection, 没让用户自定义
    // HACK: 按值捕获 conn
    conn_ptr->SetCloseCallback(
        [this, conn_ptr](TcpConnectionPtr const&) { RemoveConnection(conn_ptr); });  // 设置连接关闭后的回调函数

    // HACK: 对照 RemoveConnectionInLoop 中 sub_loop->QueueInLoop  理解
    // 在 sub_loop 中建立连接需要调用 conn->ConnectEstablished()
    // 在 sub_loop 中销毁连接需要调用 conn->ConnectDestroyed()
    // HACK: 按值捕获 conn
    sub_loop->RunInLoop([conn_ptr] { conn_ptr->ConnectEstablished(); });
}

void TcpServer::RemoveConnection(TcpConnectionPtr const& conn_ptr) {
    // 所有 TcpConnectionPtr 由 Mainloop 管理, 所以这里通过 Mainloop 删除连接
    loop_->RunInLoop([this, conn_ptr] { RemoveConnectionInLoop(conn_ptr); });
}

void TcpServer::RemoveConnectionInLoop(TcpConnectionPtr const& conn_ptr) {
    LOG_INFO("TcpServer::RemoveConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn_ptr->GetName().c_str());
    connections_.erase(conn_ptr->GetName());
    // HACK: 先获取当前连接的 Subloop
    auto sub_loop{conn_ptr->GetLoop()};
    // 再将 TcpConnection 的连接销毁函数放入 Subloop 的任务队列
    sub_loop->QueueInLoop([conn_ptr] { conn_ptr->ConnectDestroyed(); });
}

void TcpServer::SetThreadNum(int num_threads) {
    num_threads_ = num_threads;
    thread_pool_->SetThreadNum(num_threads);
}

void TcpServer::SetThreadInitCallback(ThreadInitCallback cb) {
    thread_init_callback_ = std::move(cb);
}

void TcpServer::SetConnectionCallback(ConnectionCallback cb) {
    connection_callback_ = std::move(cb);
}

void TcpServer::SetMessageCallback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}

void TcpServer::SetWriteCompleteCallback(WriteCompleteCallback cb) {
    write_complete_callback_ = std::move(cb);
}

}  // namespace cutemuduo
