#include "timerqueue.h"
#include "EventLoop.h"
#include "channel.h"
#include "timer.h"
#include "util.h"
#include <cassert>
#include <cstdint>
#include <ctime>
#include <functional>
#include <sys/timerfd.h>
#include <unistd.h>
#include <vector>

TimerFd TimerFd::createTimerFd()
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    errif(fd < 0, "Failed to create timerfd");
    return TimerFd(fd);
}

int TimerFd::setTime(int flag, const itimerspec* newTime, itimerspec* oldTime)
{
    return timerfd_settime(fd_, flag, newTime, oldTime);
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop), timefd_(TimerFd::createTimerFd()), channel_(&timefd_), nextExpire_(TimeSpec::inValidExpired),
      callingExpiredTimers_(false)
{
    channel_.setReadCb(std::bind(&TimerQueue::handleRead, this));
    channel_.enableRead();
    channel_.enableEt();
    channel_.start();
    loop_->updateChannel(&channel_);
}

TimerQueue::~TimerQueue() = default;

// 加入定时器，可能在不同线程调用
TimerId TimerQueue::addTimer(const std::function<void()>& cb, const TimeSpec& when, double interval)
{
    Timer* rawPtr = new Timer(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, rawPtr));
    return TimerId(rawPtr, rawPtr->id());
}

// 只会在属于的loop中执行
void TimerQueue::addTimerInLoop(Timer* rawPtr)
{
    TimerPtr timer(rawPtr);
    timers_[TimerIndex(rawPtr->expired(), rawPtr->id())] = std::move(timer);
}

void TimerQueue::handleRead()
{

    auto now = TimeSpec::getNow();
    // 读出定时器
    uint64_t count = 0;
    ssize_t n = timefd_.read(&count, sizeof(count));
    errif(n != sizeof(count), "Failed to read timerfd");

    callingExpiredTimers_ = true;
    removedTimers_.clear();
    std::vector<TimerPtr> expiredTimers;
    getExpiredTimers(now, expiredTimers);

    for (auto& timer : expiredTimers)
    {
        timer->handleEvent();
    }
    callingExpiredTimers_ = false;

    // 移除被激活的定时器
    for (auto& timer : expiredTimers)
    {
        // 不在移除列表
        if (timer->isRepeat() && removedTimers_.find(timer->id()) == removedTimers_.end())
        {
            timer->restart();
            timers_[TimerIndex(timer->expired(), timer->id())] = std::move(timer);
        }
    }
}

// 统一在事件处理完调用
void TimerQueue::updateTimerFd()
{
    if (timers_.empty())
    {
        // 定时器自动停止
        nextExpire_ = TimeSpec::inValidExpired;
        return;
    }
    // 第一个定时器的到期时间
    TimeSpec earlyExpired = timers_.begin()->second->expired();
    // 有更早的定时器事件
    if (earlyExpired != nextExpire_)
    {
        itimerspec temp;
        temp.it_value = earlyExpired;
        // 周期设为0，表示只执行一次
        temp.it_interval.tv_sec = 0;
        temp.it_interval.tv_nsec = 0;
        timefd_.setTime(TFD_TIMER_ABSTIME, &temp, nullptr);
        nextExpire_ = earlyExpired;
    }
}

void TimerQueue::removeTimer(TimerId& timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::removeTimerInLoop, this, timerId));
}

void TimerQueue::removeTimerInLoop(TimerId timerId)
{
    TimerIndex index(timerId.timer_->expired(), timerId.timer_->id());
    auto target = timers_.find(index);
    if (target != timers_.end())
    {
        timers_.erase(target);
    }
    else if (callingExpiredTimers_)
    {
        removedTimers_.insert(timerId.id_);
    }
}

void TimerQueue::getExpiredTimers(const TimeSpec& now, std::vector<TimerPtr>& expiredTimers)
{
    TimerIndex index(now, UINT64_MAX);
    auto bound = timers_.lower_bound(index);
    for (auto it = timers_.begin(); it != bound;)
    {
        expiredTimers.push_back(std::move(it->second));
        it = timers_.erase(it);
    }
}
