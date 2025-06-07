#pragma once

#include <functional>
namespace tcp
{

class Channel
{
  public:
    using Task = std::function<void()>;
    enum class Type
    {
        kNone,
        kRead,
        kWrite,
        kBoth,
    };
    Channel();
    ~Channel();
    void start();
    void stop();
    auto fd() -> int;
    void setFd(int fd);
    auto type() -> Type;
    void enableRead();
    void enableWrite();
    void disableRead();
    void disableWrite();
    void disableAll();
    bool inThread();
    void setReadTask(Task&& task);
    void setWriteTask(Task&& task);
    void onEvent();
    void setExpiredType(Type type);
    void setType(Type type);
    auto isStop() -> bool;
    void runTask(Task&& task, double delay = 0.0, double interval = 0.0);
    // FIFO
    void addTask(Task&& task);

  private:
    int fd_;
    Task read_task_;
    Task write_task_;
    Type type_;
    Type expired_type_;
    bool isStop_;
};

} // namespace tcp