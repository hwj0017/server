#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

#define LOGPATH "./log/1.log"

namespace utils
{
class Logger
{
  public:
    enum Target
    {
        Terminal,
        File,
        Both
    };
    Logger(const char* path, Target target = Both);
    ~Logger();
    template <typename T> Logger& operator<<(T&& text);

    static Logger logger;

  private:
    // 将日志输出到文件的流对象
    std::ofstream outfile_;
    // 日志文件路径
    const std::string path_;
    Target target_;
    std::thread writeThread_;
    // 互斥锁
    std::mutex mutex_;
    std::queue<std::string> queue_;

    const char* time();
    void writeThreadFunc();
};

template <typename T> Logger& Logger::operator<<(T&& text)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (target_ != File)
    {
        std::cout << std::forward<T>(text) << std::endl;
    }

    if (target_ != Terminal)
    {
        queue_.push(text + "\n");
    }
    return *this;
}

} // namespace utils
