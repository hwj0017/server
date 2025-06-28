#include "iocontextpool.h"

#include "iocontext.h"
#include "iocontextthread.h"
#include <memory>
#include <thread>

namespace tcp
{

IoContextPool::IoContextPool() : threads_(IoContextCount)
{
    for (auto& io_context_thread : threads_)
    {
        io_context_thread = std::make_unique<IoContextThread>();
    }
}

IoContextPool::~IoContextPool() = default;

void IoContextPool::run()
{
    for (auto& io_context_thread : threads_)
    {
        auto [thread_id, io_context] = io_context_thread->start();
        io_contexts_.push_back(io_context);
        io_context_map_.emplace(std::hash<std::thread::id>{}(thread_id), io_context);
    }
}

IoContext* IoContextPool::getIoContext() const { return io_contexts_[next_io_context_id_++]; }

IoContext* IoContextPool::getCurrentThreadIoContext() const
{
    auto it = io_context_map_.find(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return it->second;
}

} // namespace tcp