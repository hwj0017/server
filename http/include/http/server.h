#pragma once
#include <memory>

namespace http
{
// TODO: just get file
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