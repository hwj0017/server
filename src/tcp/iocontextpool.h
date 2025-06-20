#pragma once

#include "tcp/acceptor.h"
namespace tcp
{
class IoContext;
class IoContextPool
{
  public:
    IoContextPool();
    ~IoContextPool();
    void start();
    IoContext* getIoContext();
    void getCurrentThreadIoContext();
};

} // namespace tcp