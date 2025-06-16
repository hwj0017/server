// #include "iocontext.h"
// #include "socket.h"
// #include "tcp/connector.h"
// #include "utils/task.h"
// #include <string>
// #include <unistd.h>
// namespace tcp
// {
// struct Connector::Impl
// {
//     Socket socket;
//     IoContext* io_context;
//     IoContext::IoChannel io_channel;
//     Impl(const InetAddress& server_address)
//         : socket(Socket::createConnectorSocket(server_address)), io_context(), io_channel(socket.fd(), io_context)
//     {
//     }
//     ~Impl() = default;
// };
// auto Connector::async_read() -> utils::Task<std::string>
// {
//     co_await impl_->io_context->in(&impl_->io_channel);
//     co_return impl_->socket.recv();
// }
// auto Connector::async_send(std::string_view data) -> utils::Task<size_t>
// {
//     size_t total_size = 0;
//     auto write_size = impl_->socket.send(data);
//     total_size += write_size;
//     data.remove_prefix(write_size);
//     std::string left_data(data);
//     std::string_view left_view(left_data);
//     while (!left_view.empty())
//     {
//         co_await impl_->io_context->out(&impl_->io_channel);
//         auto write_size = impl_->socket.send(left_view);
//         total_size += write_size;
//         left_view.remove_prefix(write_size);
//     }
//     co_return total_size;
// }
// } // namespace tcp