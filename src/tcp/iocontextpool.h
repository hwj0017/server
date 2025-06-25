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
    IoContext* getIoContext();
    void getCurrentThreadIoContext();

  private:
    std::unique_ptr<IoContext> io_context_;
};

} // namespace tcp