#pragma once

#include "tcp/acceptor.h"
#include "tcp/connector.h"
#include "utils/task.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string_view>
namespace tcp
{

class Server
{
  public:
    Server();
    ~Server();

    // use Server::start in last
    void start();
    auto new_acceptor(std::string_view listen_ip, uint16_t port) -> std::shared_ptr<Acceptor>;
    auto new_connector(std::string_view server_ip, uint16_t port) -> std::shared_ptr<Connector>;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp