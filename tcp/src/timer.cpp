#include "timer.h"
#include "iocontext.h"
#include "ionode.h"
#include "timernode.h"
#include "utils/task.h"
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <sys/timerfd.h>
#include <unistd.h>
#include <vector>

namespace tcp
{

Timer::Timer(IoContext* io_context)
    : io_context_(io_context), timefd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)), io_node_(timefd_),
      nextExpire_(TimeSpec::inValidExpired)
{
    assert(timefd_ != -1);
}

void Timer::start()
{
    io_node_.type = IoNode::Type::Read;
    io_context_->add(&io_node_);
    io_node_.on_read_ = [this]() { on_read(); };
}
Timer::~Timer()
{
    io_context_->remove(&io_node_);
    ::close(timefd_);
}

void Timer::on_read()
{
    auto now = TimeSpec::getNow();
    // 读出定时器
    uint64_t count = 0;
    ssize_t n = ::read(timefd_, &count, sizeof(count));
    assert(n == sizeof(count));
    std::vector<std::unique_ptr<TimerNode>> temp_nodes;
    // 遍历定时器
    for (auto it = time_queue_.begin(); it != time_queue_.end() && (*it)->expired_time_ <= now;)
    {
        auto id = (*it)->task_.id();
        it = time_queue_.erase(it);
        auto it_1 = timer_nodes_.find(id);
        assert(it_1 != timer_nodes_.end());
        temp_nodes.emplace_back(std::move(it_1->second));
        timer_nodes_.erase(it_1);
    }
    for (auto& node : temp_nodes)
    {
        node->task_.resume();
    }
}

// 统一在事件处理完调用
void Timer::update()
{
    if (timer_nodes_.empty())
    {
        // 定时器自动停止
        nextExpire_ = TimeSpec::inValidExpired;
        return;
    }
    // 第一个定时器的到期时间
    TimeSpec earlyExpired = (*time_queue_.begin())->expired_time_;
    // 有更早的定时器事件
    if (earlyExpired != nextExpire_)
    {
        itimerspec temp;
        temp.it_value = earlyExpired;
        // 周期设为0，表示只执行一次
        temp.it_interval.tv_sec = 0;
        temp.it_interval.tv_nsec = 0;
        ::timerfd_settime(timefd_, TFD_TIMER_ABSTIME, &temp, nullptr);
        nextExpire_ = earlyExpired;
    }
}
auto Timer::add_delay(utils::BaseIdTask task, double delay) -> utils::Task<>
{
    co_await io_context_->in_thread();
    auto expired_time = TimeSpec::getNow() + delay;
    auto timer_node = std::make_unique<TimerNode>(std::move(task), expired_time);
    time_queue_.emplace(timer_node.get());
    timer_nodes_.emplace(timer_node->task_.id(), std::move(timer_node));
}

auto Timer::cancel_delay(size_t id) -> utils::Task<>
{
    co_await io_context_->in_thread();
    auto it = timer_nodes_.find(id);
    if (it == timer_nodes_.end())
    {
        co_return;
    }
    time_queue_.erase(it->second.get());
    timer_nodes_.erase(it);
}
} // namespace tcp