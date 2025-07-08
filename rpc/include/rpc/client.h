#pragma once

#include "common.pb.h"
#include "utils/task.h"
#include <iostream>
#include <string>

namespace rpc
{
class Client
{

  public:
    // using Request = ::rpc::Request;
    // using Response = ::rpc::Response;
    Client(std::string_view server_ip, uint16_t server_port);
    ~Client();
    void start();
    // 目前输入输出为定义好的input和output
    template <typename R, typename Arg> auto call(const std::string& method, Arg&& input) -> utils::Task<R>;

  private:
    auto call_impl(Request&& request) -> utils::Task<Response>;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
template <typename R, typename Arg> auto Client::call(const std::string& method, Arg&& input) -> utils::Task<R>
{
    Request request;
    request.set_method(method);
    request.set_input(input.SerializeAsString());
    // std::cout << input.SerializeAsString() << std::endl;
    auto response = co_await call_impl(std::move(request));

    R output;
    if (response.has_value())
    {
        // std::cout << response.value().output() << std::endl;
        if (response.value().method() == method)
        {
            output.ParseFromString(response.value().output());
        }
    }
    co_return output;
}

} // namespace rpc
