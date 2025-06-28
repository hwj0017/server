#include "iocontext.h"
#include "tcp/awaitables.h"
#include "utils/task.h"
namespace tcp
{

void Delay::await_suspend(utils::BaseIdTask&& task) { io_context_->add_delay(std::move(task), delay); }
} // namespace tcp