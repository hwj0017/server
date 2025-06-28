#pragma once

#include "utils/task.h"
#include <atomic>
#include <cstdint>
#include <ctime>
#include <functional>
namespace tcp
{
// 加入比较函数
class TimeSpec : public timespec
{
  public:
    TimeSpec() : timespec() {}
    TimeSpec(const timespec& ts) : timespec(ts) {}

    bool operator<(const TimeSpec& other) const
    {
        if (tv_sec != other.tv_sec)
            return tv_sec < other.tv_sec;
        else
            return tv_nsec < other.tv_nsec;
    }
    bool operator<=(const TimeSpec& other) const { return operator<(other) || operator==(other); }
    bool operator!=(const TimeSpec& other) const { return tv_sec != other.tv_sec || tv_nsec != other.tv_nsec; }

    bool operator==(const TimeSpec& other) const { return !(*this != other); }
    TimeSpec operator+(const TimeSpec& other) const
    {
        return TimeSpec({tv_sec + other.tv_sec, tv_nsec + other.tv_nsec});
    }

    TimeSpec operator+(double delay) const
    {
        return TimeSpec({tv_sec + static_cast<time_t>(delay),
                         tv_nsec + static_cast<long>((delay - static_cast<time_t>(delay)) * 1e9) % 1000000000});
    }
    // 获取当前时间
    static TimeSpec getNow()
    {
        TimeSpec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now;
    }
    // 无效时间
    const static TimeSpec inValidExpired;
};
inline const TimeSpec TimeSpec::inValidExpired{};

struct TimerNode
{
    struct Less
    {
        bool operator()(const TimerNode* lhs, const TimerNode* rhs) const
        {
            if (lhs->expired_time_ != rhs->expired_time_)
            {
                return lhs->expired_time_ < rhs->expired_time_;
            }
            return lhs->task_.id() < rhs->task_.id();
        }
    };
    TimerNode(utils::BaseIdTask&& task, TimeSpec expired_time) : task_(std::move(task)), expired_time_(expired_time) {}
    TimerNode(TimerNode&&) = default;
    utils::BaseIdTask task_;
    TimeSpec expired_time_;
};

} // namespace tcp
