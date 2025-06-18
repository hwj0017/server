#pragma once

#include "tcp/acceptor.h"
#include "tcp/connector.h"
#include "utils/task.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace tcp
{

class Server
{
  public:
    using ServerTask = std::function<utils::Task<>(Server&)>;
    Server();
    ~Server();
    void start(ServerTask task);
    auto new_acceptor(std::string_view listen_ip, uint16_t port) -> std::shared_ptr<Acceptor>;
    auto new_connector(std::string_view server_ip, uint16_t port) -> std::shared_ptr<Connector>;
};

} // namespace tcp