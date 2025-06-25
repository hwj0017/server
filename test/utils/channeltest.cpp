#include "utils/channel.h"
#include "utils/task.h"
#include <iostream>

int main()
{
    utils::Channel<> channel(0, 0);
    [&channel]() -> utils::Task<> {
        co_await channel.async_pop();
        std::cout << "pop" << std::endl;
    }();
    // channel.tasks_1_.front().resume();
    // [&channel]() -> utils::Task<> { co_await channel.push
    channel.push();
}