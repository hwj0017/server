#include "rpc/server.h"
#include "service/echo.h"
#include <memory>
int main()
{
    rpc::Server server("127.0.0.1", 8080);
    server.register_service("echo", &echo);
    server.start();
}