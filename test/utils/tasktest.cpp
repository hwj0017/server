
#include "utils/task.h"
#include <coroutine>
auto fun1() -> utils::IdTask<> { co_await std::suspend_always{}; }
auto fun2() -> utils::IdTask<>
{
    fun1();
    co_await std::suspend_always{};
}
int main()
{
    fun2();
    return 0;
}