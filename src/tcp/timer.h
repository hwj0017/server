// #ifndef TIMER_H
// #define TIMER_H

// #include <atomic>
// #include <cstdint>
// #include <ctime>
// #include <functional>

// class Timer;
// class TimeSpec;
// class TimerId
// {
//   public:
//     TimerId(const TimerId& other) : timer_(other.timer_), id_(other.id_)
//     {
//     }
//     // 由调用者保证线程安全
//     // void setTime(const TimeSpec& time);

//   private:
//     TimerId(Timer* timer, uint64_t id) : timer_(timer), id_(id)
//     {
//     }

//     Timer* timer_;
//     uint64_t id_;
//     friend class TimerQueue;
// };

// // 加入比较函数
// class TimeSpec : public timespec
// {
//   public:
//     TimeSpec() : timespec()
//     {
//     }
//     TimeSpec(const timespec& ts) : timespec(ts)
//     {
//     }

//     bool operator<(const TimeSpec& other) const
//     {
//         if (tv_sec != other.tv_sec)
//             return tv_sec < other.tv_sec;
//         else
//             return tv_nsec < other.tv_nsec;
//     }
//     bool operator!=(const TimeSpec& other) const
//     {
//         return tv_sec != other.tv_sec || tv_nsec != other.tv_nsec;
//     }

//     bool operator==(const TimeSpec& other) const
//     {
//         return !(*this != other);
//     }
//     TimeSpec operator+(const TimeSpec& other) const
//     {
//         return TimeSpec({tv_sec + other.tv_sec, tv_nsec + other.tv_nsec});
//     }

//     TimeSpec operator+(double delay) const
//     {
//         return TimeSpec({tv_sec + static_cast<time_t>(delay),
//                          tv_nsec + static_cast<long>((delay - static_cast<time_t>(delay)) * 1e9) % 1000000000});
//     }
//     // 获取当前时间
//     static TimeSpec getNow()
//     {
//         TimeSpec now;
//         clock_gettime(CLOCK_MONOTONIC, &now);
//         return now;
//     }
//     // 无效时间
//     const static TimeSpec inValidExpired;
// };

// class Timer
// {
//   public:
//     Timer(const std::function<void()>& cb, TimeSpec expired, double interval)
//         : cb_(cb), expired_(expired), interval_(interval), id_(count_.fetch_add(1))
//     {
//     }

//     void restart()
//     {
//         expired_.tv_sec += interval_;
//     }
//     TimeSpec expired() const
//     {
//         return expired_;
//     }
//     bool isRepeat() const
//     {
//         return interval_ > 0;
//     }
//     void handleEvent() const
//     {
//         if (cb_)
//             cb_();
//     }
//     uint64_t id() const
//     {
//         return id_;
//     }

//   private:
//     std::function<void()> cb_;
//     TimeSpec expired_;
//     double interval_;
//     const uint64_t id_;
//     static std::atomic<uint64_t> count_;
// };

// #endif // TIMER_H