#pragma once

#include <arpa/inet.h>

#include <string>

namespace cutemuduo {

class InetAddress {
public:
    explicit InetAddress(uint16_t port, std::string ip);

    explicit InetAddress(const sockaddr_in &addr);

public:
    // 返回ip地址
    std::string ToIp() const;

    // 返回ip地址和端口
    std::string ToIpPort() const;

    // 返回端口
    uint16_t ToPort() const;

    // 返回sockaddr_in结构体
    const sockaddr_in *GetSockAddr() const;

    // 设置sockaddr_in结构体
    void SetSockAddr(const sockaddr_in &addr);

private:
    sockaddr_in addr_;
};
}  // namespace cutemuduo
