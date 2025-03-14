#include <string.h>
//

#include <cutemuduo/inet_address.hpp>

namespace cutemuduo {

InetAddress::InetAddress(uint16_t port, std::string ip) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);  // inet_pton: presentation to network
}

InetAddress::InetAddress(sockaddr_in const& addr) : addr_(addr) {}

std::string InetAddress::ToIp() const {
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddress::ToIpPort() const {
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    uint16_t port = ntohs(addr_.sin_port);
    // TODO: snprintf?
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%u", port);
    return buf;
}

uint16_t InetAddress::ToPort() const {
    return ntohs(addr_.sin_port);
}

const sockaddr_in* InetAddress::GetSockAddr() const {
    return &addr_;
}

void InetAddress::SetSockAddr(sockaddr_in const& addr) {
    addr_ = addr;
}

}  // namespace cutemuduo
