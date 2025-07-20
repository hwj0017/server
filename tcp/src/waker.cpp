#include "waker.h"
#include "iocontext.h"
#include "utils/log.h"
#include "utils/task.h"
#include <algorithm>
#include <coroutine>
#include <memory>
#include <sys/eventfd.h>

namespace tcp
{

Waker::Waker(IoContext* io_context) : fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)), io_context_(io_context)
{
    if (fd_ < 0)
    {
        throw std::runtime_error("Failed to create eventfd");
    }
}
Waker::~Waker()
{
    io_context_->remove(fd_);
    if (fd_ >= 0)
    {
        ::close(fd_);
    }
}

auto Waker::start() -> utils::Task<>
{
    auto in_channel = io_context_->get_in_channel(fd_);
    while (true)
    {
        if (!co_await in_channel->async_pop())
        {
            co_return;
        }
        utils::Logger::logger << "weaker" + std::to_string(fd_) + "\n";

        uint64_t value = 1;
        ssize_t n = ::read(fd_, &value, sizeof(value));
        if (n != sizeof(value))
        {
            throw std::runtime_error("Failed to read to waker");
        }
    }
}

void Waker::wakeup()
{
    uint64_t value = 1; // Arbitrary value to wake up the context
    ssize_t n = ::write(fd_, &value, sizeof(value));
    if (n != sizeof(value))
    {
        // Handle error or unexpected write size
        throw std::runtime_error("Failed to write to waker");
    }
}
} // namespace tcp
