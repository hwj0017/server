#pragma once
#include <cstddef>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>
namespace tcp
{
class Channel;
class Epoller
{
  public:
    Epoller();
    ~Epoller();
    auto poll(int timeout = -1) -> std::vector<Channel*>;
    void update(Channel* channel);

  private:
    static constexpr std::size_t kEventSize = 1024;
    int epfd_;
    epoll_event events_[kEventSize];
};
} // namespace tcp
