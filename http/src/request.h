#pragma once

#include "enums.h"
#include "utils/timespec.h"
#include <cassert>
#include <map>
#include <string_view>

namespace http
{

// all are string_view
class Request
{
  public:
    using TimeSpec = utils::TimeSpec;

    Request() : method_(Method::Invalid), version_(Version::Unknow) {}

    Method getMethod() const { return method_; }

    void setMethod(Method method) { method_ = method; }

    bool setMethod(std::string_view method)
    {
        assert(method_ == Invalid);
        if (method == "GET")
        {
            method_ = Get;
        }
        else if (method == "POST")
        {
            method_ = Post;
        }
        else if (method == "HEAD")
        {
            method_ = Head;
        }
        else if (method == "PUT")
        {
            method_ = Put;
        }
        else if (method == "DELETE")
        {
            method_ = DeLete;
        }
        else
        {
            method_ = Invalid;
        }
        return method_ != Invalid;
    }

    const char* methodString() const
    {
        const char* result = "UNKNOW";
        switch (method_)
        {
        case Get:
            result = "GET";
            break;
        case Post:
            result = "POST";
            break;
        case Head:
            result = "HEAD";
            break;
        case Put:
            result = "PUT";
            break;
        case DeLete:
            result = "DELETE";
            break;
        default:
            break;
        }
        return result;
    }

    Version getVersion() const { return version_; }

    void setVersion(Version version) { version_ = version; }

    const std::string_view& getPath() const { return path_; }

    void setPath(std::string_view path) { path_ = path; }

    const std::string_view& getQuery() const { return query_; }

    void setQuery(std::string_view query) { query_ = query; }

    const TimeSpec& getReceiveTime() const { return receiveTime_; }

    void setReceiveTime(const TimeSpec& time) { receiveTime_ = time; }

    const std::map<std::string_view, std::string_view>& getHeaders() const { return headers_; }

    void addHeader(std::string_view field, std::string_view value) { headers_.emplace(field, value); }

    std::string_view getHeader(std::string_view field) const
    {
        std::string_view result;
        std::map<std::string_view, std::string_view>::const_iterator it = headers_.find(field);
        if (it != headers_.end())
        {
            result = it->second;
        }
        return result;
    }

    void reset()
    {
        method_ = Invalid;
        version_ = Unknow;
        path_ = {};
        query_ = {};
        receiveTime_ = TimeSpec::inValidExpired;
        headers_.clear();
        body_ = {};
    }
    std::string_view getBody() const { return body_; }

    void setBody(std::string_view body) { body_ = body; }

  private:
    Method method_;
    Version version_;
    std::string_view path_;
    std::string_view query_;
    TimeSpec receiveTime_;
    std::map<std::string_view, std::string_view> headers_;
    std::string_view body_;
};
} // namespace http
