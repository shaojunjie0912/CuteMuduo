#include <sys/socket.h>
#include <unistd.h>
//
#include <cutemuduo/inet_address.hpp>
#include <cutemuduo/socket.hpp>

namespace cutemuduo {

Socket::Socket(int sockfd) : sockfd_(sockfd) {}

Socket::~Socket() {
    close(sockfd_);
}

int Socket::sockfd() const {
    return sockfd_;
}

void Socket::BindAddress(InetAddress const& localaddr) {
    if (bind(sockfd_, localaddr.GetSockAddr(), sizeof(localaddr.GetSockAddr())) != 0) {
        // printf();
    }
}

}  // namespace cutemuduo
