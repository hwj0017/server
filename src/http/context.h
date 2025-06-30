

#pragma once

#include "http/request.h"
#include <string>
#include <string_view>
namespace http
{
class Context
{
  public:
    using TimeSpec = utils::TimeSpec;
    enum RequestParseState
    {
        ExpectRequestLine,
        ExpectHeaders,
        ExpectBody,
        GotAll,
    };

    Context() : state_(ExpectRequestLine) {}

    // default copy-ctor, dtor and assignment are fine

    // return false if any error
    bool parseRequest(std::string_view buf);

    bool gotAll() const { return state_ == GotAll; }

    void reset()
    {
        state_ = ExpectRequestLine;
        request_.reset();
    }

    const Request& request() const { return request_; }

    Request& request() { return request_; }

  private:
    bool processRequestLine(std::string_view line);

    RequestParseState state_;
    Request request_;
};
} // namespace http
