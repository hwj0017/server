#include "tcp/server.h"
#include "iocontext.h"
#include "iocontextpool.h"
#include <memory>
#include <unistd.h>
#include <unordered_map>

namespace tcp
{
struct Server::Impl
{
    IoContextPool pool_;
    Impl() {};
    ~Impl() = default;
    void start() { pool_.run(); }
    auto new_acceptor(std::string_view listen_ip, uint16_t port) -> std::shared_ptr<Acceptor>
    {
        return std::make_shared<Acceptor>(listen_ip, port, pool_.getIoContext(), &pool_);
    }
    auto new_connector(std::string_view server_ip, uint16_t port) -> std::shared_ptr<Connector>
    {
        return std::make_shared<Connector>(server_ip, port, pool_.getCurrentThreadIoContext());
    }
};

Server::Server() : impl_(std::make_unique<Impl>()) {}
Server::~Server() = default;

void Server::start()
{
    serve();
    impl_->start();
}

auto Server::new_acceptor(std::string_view listen_ip, uint16_t port) -> std::shared_ptr<Acceptor>
{
    return impl_->new_acceptor(listen_ip, port);
}

auto Server::new_connector(std::string_view server_ip, uint16_t port) -> std::shared_ptr<Connector>
{
    return impl_->new_connector(server_ip, port);
}
} // namespace tcp