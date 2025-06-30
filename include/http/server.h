#pragma once
#include "http/request.h"
#include "http/response.h"
#include <functional>
#include <memory>

namespace http
{

class Server
{
  public:
    Server(std::string ip = "127.0.0.1", uint16_t port = 8080, std::string root = ".");
    ~Server();
    void start();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace http