#pragma once

#include <arpa/inet.h>

#include <string>

namespace cutemuduo {

// sockaddr_in 结构体的封装
class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");  // TODO: 默认值选择?

    explicit InetAddress(const sockaddr_in &addr);

public:
    // 返回ip地址
    std::string ToIp() const;

    // 返回ip地址和端口
    std::string ToIpPort() const;

    // 返回端口
    uint16_t ToPort() const;

    // 返回sockaddr_in结构体
    sockaddr_in const *GetSockAddr() const;

    // 设置sockaddr_in结构体
    void SetSockAddr(const sockaddr_in &addr);

private:
    sockaddr_in addr_;
};

}  // namespace cutemuduo
