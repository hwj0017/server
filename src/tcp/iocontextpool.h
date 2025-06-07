#pragma once

#include "iocontextthread.h"
#include <cstddef>
#include <unordered_map>
#include <vector>
namespace tcp
{
using size_t = std::size_t;
class IoContextPool
{
  public:
    enum class State
    {
        kStarted,
        kStopped
    };
    constexpr static size_t kDefaultIoContextNum = 4;
    IoContextPool(std::size_t ioContextNum = kDefaultIoContextNum);
    ~IoContextPool();
    void start();
    void stop();
    void setIoContextNum(std::size_t ioContextNum);
    std::size_t getIoContextNum() const;
    IoContext* getCurrentIoContext();

    IoContext* getIoContext();
    // IoContext* getAttachedIoContext(std::size_t baseObjectId);
    // void runTask(std::)

  private:
    std::size_t ioContextNum_;
    IoContext mainIoContext_;
    std::vector<IoContextThread> ioContexts_;
    // 对应线程id到IoContext的映射
    std::unordered_map<size_t, IoContext*> indexMap_;
    State state_;
};
} // namespace tcp