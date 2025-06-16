#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
namespace tcp
{
class InetAddress
{
  public:
    InetAddress(const char* ip, uint16_t port)
    {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &addr_.sin_addr) <= 0)
        {
            throw std::runtime_error("Invalid IP address");
        }
        addrLen_ = sizeof(addr_);
    }
    InetAddress(const InetAddress& other) = default;
    InetAddress() : addrLen_(sizeof(addr_)) {}
    ~InetAddress() = default;
    std::string toIpPort() const;

    sockaddr_in addr_;
    socklen_t addrLen_;
};

} // namespace tcp
