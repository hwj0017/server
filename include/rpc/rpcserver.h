#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

class RpcServer
{
  public:
    RpcServer(std::string_view listen_ip, uint16_t port);
    ~RpcServer();
    void start();
    template <typename F> void registerService(std::string_view method, F func);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
