#pragma once
#include "context.h"
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace http
{
class Response
{
    using Headers = std::map<std::string_view, std::string_view>;

  public:
    explicit Response() = default;

    auto message() -> std::string;
    void setVersion(Version version) { version_ = version; }
    void setStatusCode(StatusCode code) { statusCode_ = code; }
    void setCloseConnection(bool on) { closeConnection_ = on; }

    bool closeConnection() const { return closeConnection_; }

    void setContentType(std::string_view contentType) { addHeader(std::string_view("Content-Type"), contentType); }

    void addHeader(std::string_view key, std::string_view value) { headers_[key] = value; }

    void setBody(std::string body) { body_ = std::move(body); }

  private:
    Headers headers_;
    Version version_{Version::Http11};
    StatusCode statusCode_{StatusCode::Unknow};
    std::string body_;
    bool closeConnection_{false};
};

inline auto Response::message() -> std::string
{
    std::string message;
    message.clear();
    message += version_to_string(version_);
    message += code_to_string(statusCode_);
    message += "\r\n";
    if (closeConnection_)
    {
        message += "Connection: close\r\n";
    }
    else
    {
        message += "Content-Length: ";
        message += std::to_string(body_.size());
        message += "\r\n";
        message += "Connection: Keep-Alive\r\n";
    }

    for (const auto& header : headers_)
    {
        message += header.first;
        message += ": ";
        message += header.second;
        message += "\r\n";
    }

    message += "\r\n";
    message += body_;
    return message;
}
} // namespace http
