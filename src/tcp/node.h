#pragma once

#include "utils/task.h"
#include <functional>
#include <sys/types.h>
namespace tcp
{
struct Node
{
    enum class Type : u_int8_t
    {
        None = 0,
        Read = 1,
        Write = 2,
        Both = 3
    };

    int fd;
    Type type = Type::None;
    Type expired_type = Type::None;
    utils::Task<> read_task;
    utils::Task<> write_task;
    Node(int fd) : fd(fd) {}
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
} // namespace tcp
