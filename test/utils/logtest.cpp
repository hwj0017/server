

#include "log.h"
#include <string>
#include <unistd.h>
int main()
{
    Logger::logger << "Hello, world!\n";
    std::string s = "Hello, world!\n";
    Logger::logger << s;
    sleep(10);
}
