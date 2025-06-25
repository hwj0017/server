#pragma once

#include "iocontext.h"
#include "socket.h"
#include "tcp/acceptor.h"
#include "utils/channel.h"
#include <memory>
namespace tcp
{
struct AcceptorNode : public IoContext::Node
{
    utils::Channel<Socket> accept_channel{};
    AcceptorNode(int fd) : IoContext::Node(fd) {}
};
} // namespace tcp
