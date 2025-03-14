#include <cutemuduo/channel.hpp>
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/logger.hpp>
#include <cutemuduo/socket.hpp>
#include <cutemuduo/tcp_connection.hpp>

namespace cutemuduo {

// 检查 EventLoop 是否为空
static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (!loop) {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, std::string const& name_arg, int sockfd, InetAddress const& local_addr,
                             InetAddress const& peer_addr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name_arg),
      state_(StateE::kConnecting),
      reading_(true),
      socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr),
      high_water_mark_(64 * 1024 * 1024) {
    // HACK: 设置 channel 的回调函数, 之后 poller 通知 EventLoop 再通知 channel 感兴趣的事件发生
    // channel 回调相应的回调函数
    channel_->SetReadCallback([this](Timestamp receive_time) { this->HandleRead(receive_time); });

    channel_->SetWriteCallback([this]() { this->HandleWrite(); });

    channel_->SetCloseCallback([this]() { this->HandleClose(); });

    channel_->SetErrorCallback([this]() { this->HandleError(); });

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    socket_->SetKeepAlive(true);  // NOTE: 开启 TCP KeepAlive
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%s\n", name_.c_str(), channel_->fd(), StateToString().c_str());
}

void TcpConnection::SetState(StateE const& new_s) {
    state_ = new_s;
}

void TcpConnection::HandleRead(Timestamp receive_time) {
    int savedErrno = 0;
    ssize_t n = input_buffer_.ReadFd(channel_->fd(), &savedErrno);
    // 接收到数据
    if (n > 0) {
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
    // 客户端断开
    else if (n == 0) {
        HandleClose();
    }
    // 出错了
    else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        HandleError();
    }
}

void TcpConnection::HandleWrite() {}

void TcpConnection::HandleClose() {
    LOG_INFO("TcpConnection::HandleClose fd=%d state=%s\n", channel_->fd(), StateToString().c_str());
    SetState(StateE::kDisconnected);
    channel_->DisableAll();
    TcpConnectionPtr conn_ptr{shared_from_this()};  // 防止函数执行结束前, 对象被销毁
    connection_callback_(conn_ptr);                 // TODO: 用于通知上层应用连接状态的变化
    close_callback_(conn_ptr);                      // TODO: 用于通知 TcpServer 进行清理工作
}

void TcpConnection::HandleError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("TcpConnection::HandleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}

std::string TcpConnection::StateToString() const {
    switch (state_) {
        case StateE::kConnecting:
            return "kConnecting";
        case StateE::kConnected:
            return "kConnected";
        case StateE::kDisconnecting:
            return "kDisconnecting";
        case StateE::kDisconnected:
            return "kDisconnected";
        default:
            return "unknown state";
    }
}

void TcpConnection::Send(std::string const& msg) {
    // TODO:
    if (state_ == StateE::kConnected) {
        if (loop_->IsInLoopThread()) {  // 单 Reactor, 用户调用 conn->Send 时, loop_ 在当前线程
            SendInLoop(msg.c_str(), msg.size());
        } else {
            loop_->RunInLoop([this, msg] { this->SendInLoop(msg.c_str(), msg.size()); });
        }
    }
}

void TcpConnection::SendInLoop(void const* data, size_t len) {
    if (state_ == StateE::kDisconnected) {
        LOG_ERROR("disconnected, give up writing\n");
        return;
    }
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;
}

EventLoop* TcpConnection::GetLoop() const {
    return loop_;
}

std::string const& TcpConnection::GetName() const {
    return name_;
}

InetAddress const& TcpConnection::GetLocalAddress() const {
    return local_addr_;
}

InetAddress const& TcpConnection::GetPeerAddress() const {
    return peer_addr_;
}

bool TcpConnection::IsConnected() const {
    return state_ == StateE::kConnected;
}

void TcpConnection::ConnectEstablished() {
    SetState(StateE::kConnected);
    channel_->Tie(shared_from_this());         // NOTE: 用于保证 TcpConnection 对象在 channel 中的生命周期
    channel_->EnableReading();                 // 开启 channel 的读事件监听
    connection_callback_(shared_from_this());  // 新连接建立回调
}

void TcpConnection::ConnectDestroyed() {
    if (state_ == StateE::kConnected) {
        SetState(StateE::kDisconnected);
        channel_->DisableAll();
        connection_callback_(shared_from_this());
    }
    channel_->Remove();
}

void TcpConnection::ShutdownInLoop() {}

}  // namespace cutemuduo
