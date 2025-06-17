#pragma once
#include "channel.h"
#include "epoller.h"
#include "utils/task.h"
#include "waker.h"
#include <functional>
#include <memory>
#include <sys/select.h>
#include <unordered_map>
namespace tcp
{
class IoContext
{
  public:
    using CallBack = std::function<void()>;
    IoContext();
    ~IoContext() = default;
    struct Timeout;
    struct InThread;
    // 由调用方保证在线程内
    void add(std::unique_ptr<Channel> channel);
    void update(Channel* channel);
    // remove in the last
    void remove(Channel* channel);
    auto inThread() -> utils::Task<>;
    auto timeout(double seconds) -> Timeout;
    void run();
    void enableRead();
    void enableWrite();
    void disableRead();
    void disableWrite();
    void disableAll();

  private:
    void handleEvent(Channel* channel);
    Epoller epoller_;
    Waker waker_;
    std::unordered_map<int, std::unique_ptr<Channel>> channels_;
};

} // namespace tcp