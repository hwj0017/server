#pragma once
#include <functional>
#include <sys/types.h>

namespace tcp
{
struct Channel
{
    using CallBack = std::function<void()>;
    enum class Type : u_int8_t
    {
        None = 0,
        Read = 1,
        Write = 2,
        Both = 3
    };
    Channel(int fd) : fd(fd) {}
    int fd;
    Type type = Type::None;
    Type expired_type = Type::None;
    CallBack read_callBack;
    CallBack write_callBack;
};
inline auto operator|(Channel::Type lhs, Channel::Type rhs) -> Channel::Type
{
    return static_cast<Channel::Type>(static_cast<u_int8_t>(lhs) | static_cast<u_int8_t>(rhs));
}

inline auto operator&(Channel::Type lhs, Channel::Type rhs) -> Channel::Type
{
    return static_cast<Channel::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs));
}

inline bool operator&&(Channel::Type lhs, Channel::Type rhs)
{
    return static_cast<Channel::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs)) != Channel::Type::None;
}
} // namespace tcp