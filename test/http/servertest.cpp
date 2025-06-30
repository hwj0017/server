#include "http/server.h"
#include "http/request.h"
#include "http/response.h"

int main()
{
    http::Server server;
    server.start();
}