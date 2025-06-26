#pragma once
#include "node.h"
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
    Node node_;
};
} // namespace tcp