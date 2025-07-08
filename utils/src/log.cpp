#include "utils/log.h"
#include <ctime>
#include <mutex>
#include <time.h>
#include <unistd.h>
namespace utils
{

Logger::Logger(const char* path, Logger::Target target) : path_(path), target_(target)
{
    outfile_.open(path_);
    writeThread_ = std::thread(&Logger::writeThreadFunc, this);
    operator<<("Logger started at " + std::string(time()));
}
Logger::~Logger()
{
    writeThread_.join();
    outfile_.close();
}
const char* Logger::time()
{
    time_t second;
    ::time(&second);
    return ctime(&second);
}

void Logger::writeThreadFunc()
{
    while (true)
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

Logger Logger::logger(LOGPATH, Terminal);
} // namespace utils