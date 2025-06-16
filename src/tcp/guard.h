// #pragma once
// namespace tcp
// {
// class IoContext;
// class Channel;
// class Guard
// {
//   public:
//     Guard(int fd, IoContext* io_context) : io_context_(io_context), fd_(fd), channel_(nullptr) {}
//     Guard(const Guard&) = delete;
//     Guard(Guard&&) = delete;
//     ~Guard();
//     int fd_;
//     IoContext* io_context_;
//     Channel* channel_ = nullptr;

//   private:
//     friend class IoContext;
// };
// } // namespace tcp