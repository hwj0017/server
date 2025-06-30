#pragma once

#include <ctime>
namespace utils
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
} // namespace utils
