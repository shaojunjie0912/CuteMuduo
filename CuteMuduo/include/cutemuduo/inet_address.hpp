#pragma once

#include <arpa/inet.h>

#include <string>

namespace cutemuduo {

class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");

    explicit InetAddress(const sockaddr_in &addr);

public:
    std::string ToIp() const;

    std::string ToIpPort() const;

    uint16_t ToPort() const;

    const sockaddr_in *GetSockAddr() const;

    void SetSockAddr(const sockaddr_in &addr);

private:
    sockaddr_in addr_;
};
}  // namespace cutemuduo
