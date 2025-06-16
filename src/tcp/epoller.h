#pragma once
#include "channel.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
namespace tcp
{
class Channel;
class Epoller
{
  public:
    using Type = Channel::Type;
    Epoller();
    Epoller(const Epoller&) = delete;
    Epoller(Epoller&&) = delete;
    ~Epoller();
    void add(Channel* channel);
    void remove(Channel* channel);
    void update(Channel* Channel);
    auto poll() -> std::vector<Channel*>;

  private:
    static u_int32_t getEpollEvents(Type type);
    static auto getTypeFromEpollEvents(u_int32_t events) -> Type;
    static constexpr size_t kMaxEventNum_ = 1024;
    int epollfd_;
    epoll_event events_[kMaxEventNum_];
};

inline u_int32_t Epoller::getEpollEvents(Type type)
{
    u_int32_t events = EPOLLET; // Edge-triggered mode
    if (type && Type::Read)
    {
        events |= EPOLLIN;
    }
    if (type && Type::Write)
    {
        events |= EPOLLOUT;
    }
    return events;
}
inline auto Epoller::getTypeFromEpollEvents(u_int32_t events) -> Type
{
    Type type = Type::None;
    if (events & EPOLLIN)
    {
        type = type | Type::Read;
    }
    if (events & EPOLLOUT)
    {
        type = type | Type::Write;
    }
    return type;
}
} // namespace tcp