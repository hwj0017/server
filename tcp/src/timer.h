#pragma once
#include "timernode.h"
#include "utils/channel.h"
#include "utils/task.h"
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sys/time.h>
#include <type_traits>
#include <unordered_map>

namespace tcp
{

class IoContext;

class Timer
{
  public:
    using TimeSpec = utils::TimeSpec;
    Timer(IoContext* io_context);
    ~Timer();
    auto start() -> utils::Task<>;
    auto add_delay(utils::BaseIdTask task, double delay) -> utils::Task<>;
    // cancel delay
    auto cancel_delay(size_t id) -> utils::Task<>;
    // update before wait
    void update();

  private:
    int timefd_;
    IoContext* io_context_;
    // 下次触发时间，就是文件描述符的到期时间
    TimeSpec nextExpire_;
    // 存放所有定时器
    std::unordered_map<size_t, std::unique_ptr<TimerNode>> timer_nodes_;
    std::set<TimerNode*, TimerNode::Less> time_queue_;
    friend struct Delay;
    // 存放在执行handleRead中删除的定时器
    // 由addTimer插入到loop队列中
    // void removeTimerInLoop(TimerId timerId);
    // void getExpiredTimers(const TimeSpec& now, std::vector<TimerPtr>& expiredTimers);
    // static int createTimerFd();
};
} // namespace tcp