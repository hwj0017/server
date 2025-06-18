#pragma once
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
namespace tcp
{
class Epoller
{
  public:
    enum class Type : u_int8_t
    {
        None = 0,
        Read = 1,
        Write = 2,
        Both = 3
    };
    Epoller();
    Epoller(const Epoller&) = delete;
    Epoller(Epoller&&) = delete;
    ~Epoller();
    void add(int fd, Type type, void* ptr);
    void remove(int fd);
    void update(int fd, Type type, void* ptr);
    auto poll() -> std::vector<void*>;

  private:
    static u_int32_t getEpollEvents(Type type);
    static auto getTypeFromEpollEvents(u_int32_t events) -> Type;
    static constexpr size_t kMaxEventNum_ = 1024;
    int epollfd_;
    epoll_event events_[kMaxEventNum_];
};
inline auto operator|(Epoller::Type lhs, Epoller::Type rhs) -> Epoller::Type
{
    return static_cast<Epoller::Type>(static_cast<u_int8_t>(lhs) | static_cast<u_int8_t>(rhs));
}

inline auto operator&(Epoller::Type lhs, Epoller::Type rhs) -> Epoller::Type
{
    return static_cast<Epoller::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs));
}

inline bool operator&&(Epoller::Type lhs, Epoller::Type rhs)
{
    return static_cast<Epoller::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs)) != Epoller::Type::None;
}
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