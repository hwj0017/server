#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>

namespace utils
{
inline const char* time()
{
    time_t second;
    ::time(&second);
    return ctime(&second);
}
class Logger
{
  public:
    enum Target
    {
        Terminal,
        File,
        Both
    };

    Logger(const char* path, Logger::Target target) : path_(path), target_(target)
    {
        outfile_.open(path_);
        writeThread_ = std::thread(&Logger::writeThreadFunc, this);
        operator<<("Logger started at " + std::string(time()));
    }
    ~Logger()
    {
        is_stop_ = true;
        writeThread_.join();
        outfile_.close();
    }

    template <typename T> Logger& operator<<(T&& text);
    constexpr static auto LOGPATH = "./log/1.log";
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
    bool is_stop_ = false;
    void writeThreadFunc()
    {
        while (!is_stop_)
        {
            sleep(5);
            std::lock_guard<std::mutex> lock(mutex_);
            while (!queue_.empty())
            {
                outfile_ << std::move(queue_.front());
                queue_.pop();
            }
            outfile_.flush();
        }
    }
};

template <typename T> Logger& Logger::operator<<(T&& text)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (target_ != File)
    {
        std::cout << std::forward<T>(text);
    }

    if (target_ != Terminal)
    {
        queue_.push(text);
    }
    return *this;
}

inline Logger Logger::logger(LOGPATH, Terminal);
} // namespace utils
