#pragma once

#include "iocontextthread.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace tcp
{

class IoContext;
class IoContextPool
{
  public:
    IoContextPool();
    ~IoContextPool();
    void run();
    // not thread safe
    auto getIoContext() const -> IoContext*;
    // thread safe
    auto getCurrentThreadIoContext() const -> IoContext*;

  private:
    static constexpr size_t IoContextCount = 4;
    IoContext main_io_context_;
    std::vector<std::unique_ptr<IoContextThread>> threads_;
    std::vector<IoContext*> io_contexts_;
    std::unordered_map<size_t, IoContext*> io_context_map_;
    static size_t next_io_context_id_;
};
inline size_t IoContextPool::next_io_context_id_ = 0;
} // namespace tcp