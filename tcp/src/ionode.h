#pragma once

#include "utils/task.h"
#include <functional>
#include <sys/types.h>

namespace tcp
{

struct IoNode
{
    using Callback = std::function<void()>;
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
    bool is_closed = false;
    Callback on_read_;
    Callback on_write_;
    IoNode(int fd) : fd(fd) {}

}; // namespace tcp
inline auto operator|(IoNode::Type lhs, IoNode::Type rhs)
{
    return static_cast<IoNode::Type>(static_cast<u_int8_t>(lhs) | static_cast<u_int8_t>(rhs));
}
inline auto operator|=(IoNode::Type& lhs, IoNode::Type rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline auto operator&(IoNode::Type lhs, IoNode::Type rhs)
{
    return static_cast<IoNode::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs));
}
inline auto operator&=(IoNode::Type& lhs, IoNode::Type rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

inline auto operator&&(IoNode::Type lhs, IoNode::Type rhs)
{
    return static_cast<IoNode::Type>(static_cast<u_int8_t>(lhs) & static_cast<u_int8_t>(rhs)) != IoNode::Type::None;
}
} // namespace tcp