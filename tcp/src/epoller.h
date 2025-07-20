#pragma once
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
namespace tcp
{
struct Node
{
    enum class Type : u_int8_t
    {
        None = 0,
        Read = 1,
        Write = 2,
        Both = 3,
    };

    int fd;
    Type type = Type::None;
    Type expired_type = Type::None;
    Node(int fd) : fd(fd) {}
};
class Epoller
{
  public:
    Epoller();
    Epoller(const Epoller&) = delete;
    Epoller(Epoller&&) = delete;
    ~Epoller();
    void add(Node* node);
    void remove(Node* node);
    void update(Node* node);
    auto poll() -> std::vector<Node*>;

  private:
    static u_int32_t getEpollEvents(Node::Type type);
    static auto getTypeFromEpollEvents(u_int32_t events) -> Node::Type;
    static constexpr size_t kMaxEventNum_ = 1024;
    int epollfd_;
    epoll_event events_[kMaxEventNum_];
};
inline auto operator|(Node::Type lhs, Node::Type rhs)
{
    return static_cast<Node::Type>(static_cast<u_int8_t>(lhs) | static_cast<u_int8_t>(rhs));
}
inline auto operator|=(Node::Type& lhs, Node::Type rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline auto operator&(Node::Type lhs, Node::Type rhs)
{
    return static_cast<Node::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs));
}
inline auto operator&=(Node::Type& lhs, Node::Type rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline auto operator&&(Node::Type lhs, Node::Type rhs)
{
    return static_cast<Node::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs)) != Node::Type::None;
}
inline u_int32_t Epoller::getEpollEvents(Node::Type type)
{
    u_int32_t events = EPOLLET; // Edge-triggered mode
    if (type && Node::Type::Read)
    {
        events |= EPOLLIN;
    }
    if (type && Node::Type::Write)
    {
        events |= EPOLLOUT;
    }
    return events;
}
inline auto Epoller::getTypeFromEpollEvents(u_int32_t events) -> Node::Type
{
    Node::Type type = Node::Type::None;
    if (events & EPOLLIN)
    {
        type = type | Node::Type::Read;
    }
    if (events & EPOLLOUT)
    {
        type = type | Node::Type::Write;
    }
    return type;
}
} // namespace tcp