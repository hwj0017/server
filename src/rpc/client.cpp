

#include "rpcclient.h"
#include "InetAddress.h"
#include "Socket.h"
#include "service.pb.h"

RpcClient::RpcClient() : socket_(Socket::createBlocking())
{
}

void RpcClient::connect(const InetAddress& serverAddr)
{
    socket_.connect(serverAddr);
}
