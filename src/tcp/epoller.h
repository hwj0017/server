#pragma once
#include <cstddef>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>
namespace tcp
{
class Epoller
{
  public:
    enum class Type
    {
        kNone,
        kReadable,
        kWriteable,
        kBoth
    };
    using Action = std::pair<int, Type>;
    Epoller();
    ~Epoller();
    auto poll(int timeout = -1) -> std::vector<Action>;
    void update(int fd, Type type);

  private:
    epoll_event getEvent(int fd, Type type);
    static constexpr std::size_t kEventSize = 1024;
    int epfd_;
    epoll_event events_[kEventSize];
    std::unordered_map<int, Type> attachedFds_;
};
} // namespace tcp
