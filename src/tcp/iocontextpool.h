#pragma once

#include "tcp/acceptor.h"
namespace tcp
{
class IoContextPool
{
  public:
    IoContextPool();
    ~IoContextPool();
    void start();
    void getIoContext();
    void getCurrentThreadIoContext();
};

} // namespace tcp