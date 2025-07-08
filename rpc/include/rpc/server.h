#pragma once

#include "tcp/server.h"
#include <functional>
#include <google/protobuf/message.h>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace rpc
{
class Server
{
  public:
    Server(std::string_view listen_ip, uint16_t port);
    ~Server();
    void start();
    template <typename F> void register_service(std::string method, F func);

  private:
    using Message = ::google::protobuf::Message;
    template <typename R, typename Arg> static auto wrap(R (*func)(Arg), std::string&& arg) -> std::string;

    template <typename R, typename Arg>
    static auto wrap(const std::function<R(Arg)>& func, std::string&& args) -> std::string;
    void register_service_impl(std::string method, std::function<std::string(std::string)> func);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

template <typename F> void Server::register_service(std::string method, F func)
{
    register_service_impl(method, [func = std::move(func)](std::string args) { return wrap(func, std::move(args)); });
}
template <typename R, typename Arg> auto Server::wrap(R (*func)(Arg), std::string&& arg) -> std::string
{
    return wrap(std::function<R(Arg)>(func), std::move(arg));
}

template <typename R, typename Arg>
auto Server::wrap(const std::function<R(Arg)>& func, std::string&& args) -> std::string
{
    // 判断子类
    static_assert(std::is_base_of_v<Message, Arg>, "Arg error");
    static_assert(std::is_base_of_v<Message, R>, "R error");
    Arg arg;
    static_cast<Message*>(&arg)->ParseFromString(std::move(args));
    auto res = func(arg);
    return static_cast<Message*>(&res)->SerializeAsString();
}
} // namespace rpc
