#pragma once
#include "ionode.h"
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
    void start();
    void wakeup();

  private:
    auto clean() -> utils::Task<>;
    int fd_;
    IoContext* io_context_;
    IoNode node_;
};
} // namespace tcp