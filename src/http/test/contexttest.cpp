#include "./../context.h"
int main()
{
    char buf[] = "GET /HEELO HTTP/1.1\r\nHost : 127.0.0.1 : 1234\r\n Connection : Keep -alive\r\n Content -Length : "
                 "12\r\n\r\n hello world ";
    http::Context context;
    context.parseRequest(buf);
    return 0;
}