#include "iocontextpool.h"

#include "iocontext.h"
#include <memory>

namespace tcp
{

IoContextPool::IoContextPool() : io_context_(std::make_unique<IoContext>()) {}

IoContextPool::~IoContextPool() = default;

void IoContextPool::run() { io_context_->run(); }

IoContext* IoContextPool::getIoContext() { return io_context_.get(); }

} // namespace tcp