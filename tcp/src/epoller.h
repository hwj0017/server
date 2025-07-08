#pragma once
#include "ionode.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
namespace tcp
{

class Epoller
{
  public:
    Epoller();
    Epoller(const Epoller&) = delete;
    Epoller(Epoller&&) = delete;
    ~Epoller();
    void add(IoNode* node);
    void remove(IoNode* node);
    void update(IoNode* node);
    auto poll() -> std::vector<IoNode*>;

  private:
    static u_int32_t getEpollEvents(IoNode::Type type);
    static auto getTypeFromEpollEvents(u_int32_t events) -> IoNode::Type;
    static constexpr size_t kMaxEventNum_ = 1024;
    int epollfd_;
    epoll_event events_[kMaxEventNum_];
};

inline u_int32_t Epoller::getEpollEvents(IoNode::Type type)
{
    u_int32_t events = EPOLLET; // Edge-triggered mode
    if (type && IoNode::Type::Read)
    {
        events |= EPOLLIN;
    }
    if (type && IoNode::Type::Write)
    {
        events |= EPOLLOUT;
    }
    return events;
}
inline auto Epoller::getTypeFromEpollEvents(u_int32_t events) -> IoNode::Type
{
    IoNode::Type type = IoNode::Type::None;
    if (events & EPOLLIN)
    {
        type = type | IoNode::Type::Read;
    }
    if (events & EPOLLOUT)
    {
        type = type | IoNode::Type::Write;
    }
    return type;
}
} // namespace tcp