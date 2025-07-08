
#include "utils/task.h"
#include "utils/channel.h"
#include <coroutine>

class Test
{
  public:
    Test() = default;
    ~Test() { std::cout << "~Test" << std::endl; }
};

auto fun1(utils::Channel<>* channel) -> utils::IdTask<>
{
    std::cout << "fun1" << std::endl;
    co_await channel->async_pop();
    co_await std::suspend_always{};
}
auto fun2(utils::Channel<>* channel) -> utils::IdTask<>
{
    Test test;
    co_await fun1(channel);
    std::cout << "fun2" << std::endl;
}
int main()
{
    utils::Channel<> channel{1};
    fun2(&channel);
    channel.push();
    return 0;
}