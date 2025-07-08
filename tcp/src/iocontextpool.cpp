#include "iocontextpool.h"

#include "iocontext.h"
#include "iocontextthread.h"
#include <cassert>
#include <memory>
#include <thread>
namespace tcp
{

IoContextPool::IoContextPool() : threads_(IoContextCount)
{
    io_contexts_.reserve(IoContextCount);
    io_contexts_.emplace_back(&main_io_context_);
    io_context_map_.emplace(std::hash<std::thread::id>{}(std::this_thread::get_id()), &main_io_context_);
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
        io_contexts_.emplace_back(io_context);
        io_context_map_.emplace(std::hash<std::thread::id>{}(thread_id), io_context);
    }
    main_io_context_.run();
}

IoContext* IoContextPool::getIoContext() const
{
    auto io_context = io_contexts_[next_io_context_id_];
    next_io_context_id_ = (next_io_context_id_ + 1) % io_contexts_.size();
    return io_context;
}

IoContext* IoContextPool::getCurrentThreadIoContext() const
{
    auto it = io_context_map_.find(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    assert(it != io_context_map_.end());
    return it->second;
}
} // namespace tcp