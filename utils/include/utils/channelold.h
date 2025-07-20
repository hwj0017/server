// #pragma once

// #include "basetask.h"
// #include <cassert>
// #include <coroutine>
// #include <cstddef>
// #include <memory>
// #include <optional>
// #include <queue>

// namespace utils
// {
// // not thread safe
// // class ChannelBase;
// // TODO
// class ChannelBase
// {
//   public:
//     ChannelBase(size_t max_size, size_t init_size = 0) : empty_size_(max_size), full_size_(init_size) {}
//     ~ChannelBase() { close(); }
//     // get channel state
//     bool is_empty() const { return full_size_ <= 0; }
//     bool is_full() const { return empty_size_ <= 0; }
//     bool is_closed() const { return is_closed_; }
//     // close the channel
//     void close()
//     {
//         if (is_closed_)
//         {
//             return;
//         }
//         is_closed_ = true;
//         while (!res_tasks_.empty())
//         {
//             auto task = std::move(res_tasks_.front());
//             res_tasks_.pop();
//             task.resume();
//         }
//         while (!tasks_2_.empty())
//         {
//             auto task = std::move(tasks_2_.front());
//             tasks_2_.pop();
//             task.resume();
//         }
//         while (!tasks_3_.empty())
//         {
//             auto task = std::move(tasks_3_.front());
//             tasks_3_.pop();
//             task.resume();
//         }
//     }

//     // bool has_pop_task() const { return is_empty() && !res_tasks_.empty(); }

//     struct Empty
//     {
//         ChannelBase& channel_;
//         Empty(ChannelBase& channel) : channel_(channel) {}
//         bool await_ready() { return channel_.is_closed() || channel_.is_empty(); }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.tasks_3_.push({coro});
//         }
//         bool await_resume() { return !channel_.is_closed(); }
//     };
//     struct Full
//     {
//         ChannelBase& channel_;
//         Full(ChannelBase& channel) : channel_(channel) {}
//         bool await_ready() { return channel_.is_closed() || channel_.is_full(); }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.tasks_3_.push({coro});
//         }
//         bool await_resume() { return !channel_.is_closed(); }
//     };
//     struct NotEmpty
//     {
//         ChannelBase& channel_;
//         NotEmpty(ChannelBase& channel) : channel_(channel) {}
//         bool await_ready() { return channel_.is_closed() || !channel_.is_empty(); }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.tasks_2_.push({coro});
//         }
//         bool await_resume() { return !channel_.is_closed(); }
//     };
//     struct NotFull
//     {
//         ChannelBase& channel_;
//         NotFull(ChannelBase& channel) : channel_(channel) {}
//         bool await_ready() { return channel_.is_closed() || !channel_.is_full(); }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.tasks_2_.push({coro});
//         }
//         bool await_resume() { return !channel_.is_closed(); }
//     };

//     struct Closed
//     {
//         ChannelBase& channel_;
//         Closed(ChannelBase& channel) : channel_(channel) {}
//         bool await_ready() { return channel_.is_closed(); }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.tasks_4_.push({coro});
//         }
//         void await_resume() {}
//     };

//     // await empty, full, not_empty, not_full, closed, pushable, popable
//     auto empty() -> Empty { return {*this}; }
//     auto full() -> Full { return {*this}; }
//     auto not_empty() -> NotEmpty { return {*this}; }
//     auto not_full() -> NotFull { return {*this}; }
//     auto closed() -> Closed { return {*this}; }

//   public:
//     int full_size_;
//     int empty_size_;
//     bool is_closed_ = false;
//     // await pop or push
//     std::queue<BaseTask> res_tasks_;
//     // await not empty or not full
//     std::queue<BaseTask> tasks_2_;
//     // await empty or full
//     std::queue<BaseTask> tasks_3_;
//     // await closed
//     std::queue<BaseTask> tasks_4_;
//     friend struct Empty;
//     friend struct Full;
//     friend struct NotEmpty;
//     friend struct NotFull;
// };

// // channel<T> is a bounded channel that can hold T type elements.
// template <typename T = void> class Channel : public ChannelBase
// {
//   public:
//     Channel(size_t max_size) : ChannelBase(max_size, 0) {}
//     // TODO
//     struct AsyncPop
//     {
//         Channel<T>& channel_;
//         AsyncPop(Channel<T>& channel) : channel_(channel) {}
//         bool await_ready()
//         {
//             // increment empty size and decrement full size
//             ++channel_.empty_size_;
//             --channel_.full_size_;
//             if (channel_.is_closed())
//             { // resume Push
//                 if (channel_.is_full()&&)
//                 {
//                     auto task = std::move(channel_.res_tasks_.front());
//                     channel_.res_tasks_.pop();
//                     task.resume();
//                 }
//                 // resume NotFull
//                 if (!channel_.is_full())
//                 {
//                     while (!channel_.tasks_2_.empty())
//                     {
//                         auto task = std::move(channel_.tasks_2_.front());
//                         channel_.tasks_2_.pop();
//                         task.resume();
//                     }
//                 }
//                 // resume Empty
//                 if (channel_.is_empty())
//                 {
//                     while (!channel_.tasks_3_.empty())
//                     {
//                         auto task = std::move(channel_.tasks_3_.front());
//                         channel_.tasks_3_.pop();
//                         task.resume();
//                     }
//                 };
//             }
//             return ret;
//         }
//         template <typename promiss_type> void await_suspend(std::coroutine_handle<promiss_type> coro)
//         {
//             channel_.res_tasks_.push({coro});
//         }
//         auto await_resume() -> std::optional<T>
//         {
//             if (channel_.is_closed())
//             {
//                 return {};
//             }
//             auto res = std::move(channel_.resources_.front());
//             channel_.resources_.pop();
//             return res;
//         }
//     };
//     struct AsyncPush
//     {
//         Channel<T>& channel_;
//         T value_;
//         AsyncPush(Channel<T>& channel, T&& value) : channel_(channel), value_(std::move(value)) {}
//         bool await_ready()
//         {
//             auto ret = channel_.is_closed() || !channel_.is_full();
//             --channel_.empty_size_;
//             ++channel_.full_size_;
//             channel_.resources_.push(std::move(value_));
//             // resume Pop
//             if (channel_.is_empty())
//             {
//                 auto task = std::move(channel_.res_tasks_.front());
//                 channel_.res_tasks_.pop();
//                 task.resume();
//             }
//             // resume NotEmpty
//             if (!channel_.is_empty())
//             {
//                 while (!channel_.tasks_2_.empty())
//                 {
//                     auto task = std::move(channel_.tasks_2_.front());
//                     channel_.tasks_2_.pop();
//                     task.resume();
//                 }
//             }
//             // resume Full
//             if (channel_.is_full())
//             {
//                 while (!channel_.tasks_3_.empty())
//                 {
//                     auto task = std::move(channel_.tasks_3_.front());
//                     channel_.tasks_3_.pop();
//                     task.resume();
//                 }
//             }
//             return ret;
//         }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.res_tasks_.push({coro});
//         }
//         bool await_resume() { return !channel_.is_closed(); }
//     };
//     void reset()
//     {
//         close();
//         resources_ = {};
//         is_closed_ = false;
//     }
//     // auto push(T value) -> Push { return Push{*this, std::move(value)}; }
//     // auto pop() -> Pop { return Pop{*this}; }
//     bool push(T value)
//     {
//         if (is_closed() || is_full())
//         {
//             return false;
//         }
//         --empty_size_;
//         ++full_size_;
//         resources_.push(std::move(value));

//         if (is_empty())
//         {
//             // resume Pop
//             auto task = std::move(res_tasks_.front());
//             res_tasks_.pop();
//             task.resume();
//         }
//         else
//         {
//             // resume NotEmpty
//             while (!tasks_2_.empty())
//             {
//                 auto task = std::move(tasks_2_.front());
//                 tasks_2_.pop();
//                 task.resume();
//             }
//             // resume Full
//             if (is_full())
//             {
//                 while (!tasks_3_.empty())
//                 {
//                     auto task = std::move(tasks_3_.front());
//                     tasks_3_.pop();
//                     task.resume();
//                 }
//             }
//         }
//         return true;
//     }

//     auto pop() -> std::optional<T>
//     {
//         if (is_closed() || is_empty())
//         {
//             return std::nullopt;
//         }
//         ++empty_size_;
//         --full_size_;

//         auto res = std::move(resources_.front());
//         resources_.pop();
//         // resume NotFull
//         if (!is_full())
//         {
//             while (!tasks_2_.empty())

//             {
//                 auto task = std::move(tasks_2_.front());
//                 tasks_2_.pop();
//                 task.resume();
//             }
//         }
//         // resume Push
//         if (empty_size_ < 0 && !res_tasks_.empty())
//         {
//             auto task = std::move(res_tasks_.front());
//             res_tasks_.pop();
//             task.resume();
//         }
//         return {res};
//     }

//     auto async_pop() -> AsyncPop { return AsyncPop{*this}; }
//     auto async_push(T value) -> AsyncPush { return AsyncPush{*this, std::move(value)}; }

//   private:
//     std::queue<T> resources_;

//     friend struct Pop;
//     friend struct Push;
// };
// template <> class Channel<void> : public ChannelBase
// {
//   public:
//     Channel(size_t max_size, size_t init_size = 0) : ChannelBase(max_size, init_size) {}
//     bool push()
//     {

//         if (is_closed() || is_full())
//         {
//             return false;
//         }
//         --empty_size_;
//         ++full_size_;

//         if (is_empty())
//         {
//             auto task = std::move(res_tasks_.front());
//             res_tasks_.pop();
//             task.resume();
//         }
//         else
//         {
//             // resume NotEmpty
//             while (!tasks_2_.empty())
//             {
//                 auto task = std::move(tasks_2_.front());
//                 tasks_2_.pop();
//                 task.resume();
//             }
//             // resume Full
//             if (is_full())
//             {
//                 while (!tasks_3_.empty())
//                 {
//                     auto task = std::move(tasks_3_.front());
//                     tasks_3_.pop();
//                     task.resume();
//                 }
//             }
//         }
//         return true;
//     }

//     auto pop() -> bool
//     {
//         if (is_closed() || is_empty())
//         {

//             return false;
//         }
//         ++empty_size_;
//         --full_size_;
//         if (is_full())
//         {
//             // resume Push
//             auto task = std::move(res_tasks_.front());
//             res_tasks_.pop();
//             task.resume();
//         }
//         else
//         {
//             // resume NotFull
//             while (!tasks_2_.empty())
//             {
//                 auto task = std::move(tasks_2_.front());
//                 tasks_2_.pop();
//                 task.resume();
//             }
//             if (is_empty())
//             {
//                 // resume Empty
//                 while (!tasks_3_.empty())
//                 {
//                     auto task = std::move(tasks_3_.front());
//                     tasks_3_.pop();
//                     task.resume();
//                 }
//             }
//         }
//         return true;
//     }

//     struct AsyncPop
//     {
//         Channel& channel_;
//         AsyncPop(Channel& channel) : channel_(channel) {}
//         bool await_ready()
//         {
//             auto ret = channel_.is_closed() || !channel_.is_empty();
//             ++channel_.empty_size_;
//             --channel_.full_size_;
//             // resume Push
//             if (channel_.is_full())
//             {
//                 auto task = std::move(channel_.res_tasks_.front());
//                 channel_.res_tasks_.pop();
//                 task.resume();
//             }
//             // resume NotFull
//             if (!channel_.is_full())
//             {
//                 while (!channel_.tasks_2_.empty())
//                 {
//                     auto task = std::move(channel_.tasks_2_.front());
//                     channel_.tasks_2_.pop();
//                     task.resume();
//                 }
//             }
//             // resume Empty
//             if (channel_.is_empty())
//             {
//                 while (!channel_.tasks_3_.empty())
//                 {
//                     auto task = std::move(channel_.tasks_3_.front());
//                     channel_.tasks_3_.pop();
//                     task.resume();
//                 }
//             }
//             return ret;
//         }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.res_tasks_.push({coro});
//         }
//         bool await_resume()
//         {
//             if (channel_.is_closed())
//             {
//                 return false;
//             }

//             return true;
//         }
//     };
//     struct AsyncPush
//     {
//         Channel& channel_;
//         AsyncPush(Channel& channel) : channel_(channel) {}
//         bool await_ready()
//         {
//             auto ret = channel_.is_closed() || !channel_.is_full();
//             --channel_.empty_size_;
//             ++channel_.full_size_;
//             // resume Pop
//             if (channel_.is_empty())
//             {
//                 auto task = std::move(channel_.res_tasks_.front());
//                 channel_.res_tasks_.pop();
//                 task.resume();
//             }
//             // resume NotEmpty
//             if (!channel_.is_empty())
//             {
//                 while (!channel_.tasks_2_.empty())
//                 {
//                     auto task = std::move(channel_.tasks_2_.front());
//                     channel_.tasks_2_.pop();
//                     task.resume();
//                 }
//             }
//             // resume Full
//             if (channel_.is_full())
//             {
//                 while (!channel_.tasks_3_.empty())
//                 {
//                     auto task = std::move(channel_.tasks_3_.front());
//                     channel_.tasks_3_.pop();
//                     task.resume();
//                 }
//             }
//             return ret;
//         }
//         template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
//         {
//             channel_.res_tasks_.push({coro});
//         }
//         bool await_resume()
//         {
//             if (channel_.is_closed())
//             {
//                 return false;
//             }

//             return true;
//         }
//     };
//     auto async_pop() -> AsyncPop { return AsyncPop{*this}; }
//     auto async_push() -> AsyncPush { return AsyncPush{*this}; }
//     void reset()
//     {
//         close();
//         is_closed_ = false;
//     }

//     friend struct Pop;
//     friend struct Push;
// };
// } // namespace utils
