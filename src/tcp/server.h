#pragma once

#include <functional>
namespace tcp
{
class TaskRunner;
class Server
{
  public:
    using StartTask = std::function<void(Server&)>;
    Server();
    ~Server();
    TaskRunner* getTaskRunner() const;
    TaskRunner* getCurrentTaskRunner() const;
    void start(const StartTask&, double delay = 0.0);
};

} // namespace tcp