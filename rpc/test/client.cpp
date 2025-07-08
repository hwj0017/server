#include "rpc/client.h"
#include "service/service.pb.h"
#include "utils/task.h"
#include <iostream>
int main()
{
    rpc::Client client("127.0.0.1", 8080);
    [](rpc::Client& client) -> utils::Task<> {
        ::rpc::EchoRequest input;
        input.set_data("hello\n");
        auto res = co_await client.call<::rpc::EchoResponse>("echo", input);
        if (res.has_value())
        {
            std::cout << res.value().data() << std::endl;
        }
    }(client);
    client.start();
}