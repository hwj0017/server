#pragma once
#include "utils/channel.h"
#include "utils/task.h"
#include <coroutine>
#include <mutex>
#include <thread>
#include <vector>

namespace tcp
{

class IoContext;
class Waker
{
  public:
    Waker(IoContext* io_context);
    ~Waker();
    auto start() -> utils::Task<>;
    void wakeup();

  private:
    int fd_;
    IoContext* io_context_;
};
} // namespace tcp