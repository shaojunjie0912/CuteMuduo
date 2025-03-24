#include <cutemuduo/channel.hpp>
#include <cutemuduo/event_loop.hpp>
#include <cutemuduo/logger.hpp>
#include <cutemuduo/socket.hpp>
#include <cutemuduo/tcp_connection.hpp>

namespace cutemuduo {

// NOTE: 补充有关 RunInLoop 和 QueueInLoop 的选择 (GPT)
// 马上能做的用 RunInLoop
// 怕递归、怕回调、怕死循环的用 QueueInLoop

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
    // NOTE: TcpConnection 的构造函数中**注册** Channel 的回调函数
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

void TcpConnection::ConnectEstablished() {
    SetState(StateE::kConnected);
    channel_->Tie(shared_from_this());         // NOTE: 用于保证 TcpConnection 对象在 channel 中的生命周期
    channel_->EnableReading();                 // 开启 channel 的读事件监听(注册 EPOLLIN)
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

void TcpConnection::HandleRead(Timestamp receive_time) {
    int savedErrno = 0;
    // 从 fd 中读取数据进 input_buffer_
    ssize_t n = input_buffer_.ReadFd(channel_->fd(), &savedErrno);  // NOTE: 读数据是可读回调函数的主要任务
    // NOTE: 接收到数据后, 调用用户自定义的收到消息(数据)后的回调函数
    // 不需要加入 loop_ 的 pending_functors_ 任务队列中
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

void TcpConnection::HandleWrite() {
    if (channel_->IsWriting()) {
        int saved_errno = 0;
        // 将 output_buffer_ 中的 **可读空间中所有数据** 写入 fd
        ssize_t n = output_buffer_.WriteFd(channel_->fd(), &saved_errno);
        if (n > 0) {
            if (output_buffer_.ReadableBytes() == 0) {  // 如果此时 output_buffer_ 中的数据已经全部发送完毕
                channel_->DisableWriting();             // 关闭可写事件监听
                if (write_complete_callback_) {
                    // NOTE: 将 write_complete_callback_ 放入 loop_ 的 pending_functors_ 任务队列中
                    // HACK: 防止用户回调 write_complete_callback_ 调用 Send() 再次触发 HandleWrite() 造成递归调用栈溢出
                    loop_->QueueInLoop([this] { write_complete_callback_(shared_from_this()); });
                }
                if (state_ == StateE::kDisconnecting) {
                    ShutdownInLoop();  // 在当前 loop 中关闭连接
                }
            } else {
                LOG_ERROR("TcpConnection::HandleWrite");
            }
        } else {
            LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
        }
    }
}

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

void TcpConnection::SetConnectionCallback(ConnectionCallback cb) {
    connection_callback_ = std::move(cb);
}

void TcpConnection::SetCloseCallback(CloseCallback cb) {
    close_callback_ = std::move(cb);
}

void TcpConnection::SetMessageCallback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}

void TcpConnection::SetWriteCompleteCallback(WriteCompleteCallback cb) {
    write_complete_callback_ = std::move(cb);
}

void TcpConnection::SetHighWaterMarkCallback(HighWaterMarkCallback cb, size_t high_water_mark) {
    high_water_mark_callback_ = std::move(cb);
    high_water_mark_ = high_water_mark;
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
        } else {  // 多 Reactor, 用户调用 conn->Send 时, loop_ 不在当前线程
            // NOTE: 选 RunInLoop 原因: 如果是自己线程, 最好立即发
            loop_->RunInLoop([this, msg] { this->SendInLoop(msg.c_str(), msg.size()); });
        }
    }
}

void TcpConnection::SendInLoop(void const* data, size_t len) {
    if (state_ == StateE::kDisconnected) {  // 已经断开的连接, 不再发送数据
        LOG_ERROR("disconnected, give up writing\n");
        return;
    }
    ssize_t nwrote = 0;        // 已经发送的数据长度
    size_t remaining = len;    // 剩余要发送的数据长度
    bool fault_error = false;  // 记录是否产生过错误

    // 当 channel_ 没有注册可写事件并且 outputBuffer_ 中没有待发送数据, 则直接将 data 中的数据发送出去
    if (!channel_->IsWriting() && output_buffer_.ReadableBytes() == 0) {
        nwrote = write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            // 消息发送完毕, 调用用户自定义的发送完消息后的回调函数
            if (remaining == 0 && write_complete_callback_) {
                loop_->QueueInLoop([this] { write_complete_callback_(shared_from_this()); });
            }
        } else {  // nwrote<0
            nwrote = 0;
            if (errno != EWOULDBLOCK) {  // EWOULDBLOCK 表示非阻塞情况下没有数据后的正常返回
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true;
                }
            }
        }
    }

    // 1. output_buffer_ 中有待发送数据, 则将 data 中的数据追加到 outputBuffer_ 中, 一起发送
    // 2. 经过第一个 if 语句(output_buffer_ 中没有待发送数据)后, write 没有写完所有数据
    if (!fault_error && remaining > 0) {
        size_t old_len = output_buffer_.ReadableBytes();
        if (old_len + remaining >= high_water_mark_ && old_len < high_water_mark_ && high_water_mark_callback_) {
            // 如果要发送的数据长度超过了高水位标记, 则调用用户自定义的高水位标记回调函数
            loop_->QueueInLoop([&, this] { high_water_mark_callback_(shared_from_this(), old_len + remaining); });
        }
        // 将 data 中的数据追加到 outputBuffer_ 中
        output_buffer_.Append(static_cast<char const*>(data) + nwrote, remaining);
        if (!channel_->IsWriting()) {
            channel_->EnableWriting();  // NOTE: 开启 channel 的可写事件监听
        }
    }
}

// TODO: SendFile
// TODO: SendFileInLoop

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

void TcpConnection::Shutdown() {
    if (state_ == StateE::kConnected) {
        SetState(StateE::kDisconnecting);  // 标记 **正在** 断开连接
        // NOTE: 用 RunInLoop 原因: 尽快关闭
        loop_->RunInLoop([this] { ShutdownInLoop(); });
    }
}

void TcpConnection::ShutdownInLoop() {
    if (!channel_->IsWriting()) {  // 如果当前 channel 没有写事件, 说明 output_buffer_ 数据已经发送完毕
        socket_->ShutdownWrite();  // 调用 socket_ 的 ShutdownWrite() 关闭写端
    }
}

}  // namespace cutemuduo
