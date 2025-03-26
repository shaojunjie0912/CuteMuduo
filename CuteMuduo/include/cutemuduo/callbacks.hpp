#pragma once

#include <functional>
#include <memory>

namespace cutemuduo {

class Buffer;
class TcpConnection;
class Timestamp;

// NOTE: 每个回调都传入 TcpConnectionPtr 参数, 为了知道是 **哪个连接** 发生了事件

// 指向 TcpConnection 的智能指针
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;

using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

}  // namespace cutemuduo
