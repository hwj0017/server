#pragma once

#include "utils/task.h"
#include "utils/timespec.h"
#include <atomic>
#include <cstdint>
#include <ctime>
#include <functional>
namespace tcp
{

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
    TimerNode(utils::BaseIdTask&& task, utils::TimeSpec expired_time)
        : task_(std::move(task)), expired_time_(expired_time)
    {
    }
    TimerNode(TimerNode&&) = default;
    utils::BaseIdTask task_;
    utils::TimeSpec expired_time_;
};

} // namespace tcp
