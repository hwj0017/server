

#include "iocontextpool.h"
#include "iocontext.h"
#include <cassert>
#include <functional>
#include <thread>

namespace tcp
{

IoContextPool::IoContextPool(size_t ioContextNum)
    : ioContextNum_(ioContextNum), ioContexts_(ioContextNum_), state_(State::kStopped)
{
}

IoContextPool::~IoContextPool()
{
    for (auto& ioContextThread : ioContexts_)
    {
        ioContextThread.stop();
    }
    mainIoContext_.stop();
}
void IoContextPool::start()
{
}

// void IoContextPool::stop()
// {
// {
//     for (auto& ioContext : ioContexts_)
//     {
//         ioContext.stop();
//     }
// }

IoContext* IoContextPool::getCurrentIoContext()
{
    auto it = indexMap_.find(std::hash<std::thread::id>()(std::this_thread::get_id()));
    if (it == indexMap_.end())
    {
        return nullptr;
    }
    return it->second;
}

} // namespace tcp