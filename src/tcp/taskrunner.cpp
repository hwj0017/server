#include "taskrunner.h"
#include "epoller.h"
#include <cassert>
#include <sys/eventfd.h>
#include <sys/types.h>

namespace tcp
{

TaskRunner::TaskRunner()
    : epoller_(), waker_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)), handleState_(HandleState::Waiting),
      state_(State::kStopped)
{
    auto wakerOnRead = [fd = waker_]() {
        uint64_t count = 0;
        ssize_t readNum = read(fd, &count, sizeof(count));
        assert(readNum == sizeof(count));
    };
    addIoTask({waker_, IoType::kRead, std::move(wakerOnRead), nullptr});
}
void TaskRunner::addIoTask(IoTask&& ioTask)
{
    switch (ioTask.ioType)
    {
    case IoType::kRead:
        epoller_.update(ioTask.fd, Epoller::Type::kReadable);
        break;
    case IoType::kWrite:
        epoller_.update(ioTask.fd, Epoller::Type::kWriteable);
        break;
    case IoType::kBoth:
        epoller_.update(ioTask.fd, Epoller::Type::kBoth);
        break;
    case IoType::kNone:
        break;
    }
    ioTasks_.emplace(std::move(ioTask));
}

void TaskRunner::removeIoTask(int fd)
{
    epoller_.update(fd, Epoller::Type::kNone);
    ioTasks_.erase(fd);
}

void TaskRunner::updateIoTask(int fd, IoType ioType)
{
    if (ioTasks_.find(fd) != ioTasks_.end())
    {
        switch (ioType)
        {
        case IoType::kRead:
            epoller_.update(fd, Epoller::Type::kReadable);
            break;
        case IoType::kWrite:
            epoller_.update(fd, Epoller::Type::kWriteable);
            break;
        case IoType::kBoth:
            epoller_.update(fd, Epoller::Type::kBoth);
            break;
        case IoType::kNone:
            epoller_.update(fd, Epoller::Type::kNone);
            break;
        }
    }
}

void TaskRunner::runTask(Task&& task, double delay, double interval)
{
    if (inOwnThread())
        task();
    else
        addTask(std::move(task), delay, interval);
    Task a(
});
}

void TaskRunner::addTask(Task&& task, double delay, double interval)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace_back(std::move(task));
    }
    // 无论是不是当前线程，只要不是在处理事件都唤醒
    // if (handleState_ != HandlingEvents)
    //     waker.wakeup();
}

void TaskRunner::wakeup() const
{
    uint64_t count = 0;
    ssize_t writeNum = write(waker, &count, sizeof(count));
    assert(writeNum == sizeof(count));
}
} // namespace tcp