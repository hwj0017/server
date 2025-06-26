#pragma once

#include "tcp/acceptor.h"
#include <memory>
namespace tcp
{
class IoContext;
class IoContextPool
{
  public:
    IoContextPool();
    ~IoContextPool();
    void run();
    auto getIoContext() -> IoContext*;
    auto getCurrentThreadIoContext() -> IoContext*;

  private:
    std::unique_ptr<IoContext> io_context_;
};

} // namespace tcp