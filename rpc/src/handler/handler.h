// template <typename F> void ServiceHandler::registerService(const String& method, F&& func)
// {
//     registerServiceImpl(method, std::forward<F>(func));
// }
// template <typename R, typename... Args>
// void ServiceHandler::registerServiceImpl(const String& method, std::function<R(Args...)> func)
// {
//     // func需要保留，不能再移动给wrap
//     services_[method] = [func = std::move(func)](const Request& request) -> Response {
//         return wrap<R, Args...>(func, request);
//     };
// }
// template <typename R, typename... Args>
// auto ServiceHandler::wrap(const std::function<R(Args...)>& func, const Request& request) -> Response
// {
//     using ArgsType = std::tuple<std::decay_t<Args>...>;
//     constexpr auto size = std::tuple_size_v<std::decay_t<ArgsType>>;
//     auto args = getArgs<ArgsType>(request);
//     auto index = std::make_index_sequence<size>{};
//     auto output = invoke<R, Args..., ArgsType>(func, args, index);
//     Response Response;
//     Response.SerializeAsString() Response.set_output(output.S)
// }

// template <typename R, typename... Args>
// static auto wrap(const std::function<R(Args...)>& func, const Request& request) -> Response;
// template <typename R, typename... Args> static auto wrapImpl(const std::function<R(Args...)>& func) -> Response;
// template <typename Tuple> static auto getArgs(const Request& request) -> Tuple;
// template <typename R, typename... Args, typename Tuple, std::size_t... Index>
// static auto invoke(const std::function<R(Args...)>& func, const Tuple& args, std::index_sequence<Index...>) -> R;