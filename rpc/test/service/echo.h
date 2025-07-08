#pragma once

#include "service.pb.h"

inline ::rpc::EchoResponse echo(::rpc::EchoRequest msg)
{

    ::rpc::EchoResponse res;
    res.set_data(std::move(msg.data()));
    return res;
}
