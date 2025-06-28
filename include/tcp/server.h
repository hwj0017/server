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
    using ServerTask = std::function<utils::Task<>(Server&)>;
    Server();
    ~Server();

    // use Server::start in last
    void start();
    auto new_acceptor(std::string_view listen_ip, uint16_t port) -> std::shared_ptr<Acceptor>;
    auto new_connector(std::string_view server_ip, uint16_t port) -> std::shared_ptr<Connector>;

    auto delay(double delay) -> Delay;
    auto cancel_delay(size_t id) -> utils::Task<>;

  protected:
    virtual auto serve() -> utils::Task<> = 0;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend struct Delay;
};

} // namespace tcp