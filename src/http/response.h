#pragma once
#include <map>
#include <string>
#include <string_view>

namespace http
{
class Response
{
    using Headers = std::map<std::string_view, std::string_view>;

  public:
    enum StatusCode
    {
        Unknow,
        Ok = 200,
        MovedPermanently = 301,
        BadRequest = 400,
        NotFound = 404,
    };

    explicit Response() = default;

    auto message() -> std::string;

    void setStatusCode(StatusCode code) { statusCode_ = code; }

    void setStatusMessage(std::string_view message) { statusMessage_ = message; }

    void setCloseConnection(bool on) { closeConnection_ = on; }

    bool closeConnection() const { return closeConnection_; }

    void setContentType(std::string_view contentType) { addHeader(std::string_view("Content-Type"), contentType); }

    // FIXME: replace string_view with StringPiece
    void addHeader(std::string_view key, std::string_view value) { headers_[key] = value; }

    void setBody(std::string body) { body_ = std::move(body); }

  private:
    Headers headers_;
    StatusCode statusCode_{Unknow};
    // FIXME: add http version
    std::string_view statusMessage_;
    std::string body_;
    bool closeConnection_{false};
};

inline auto Response::message() -> std::string
{
    std::string message;
    message.clear();
    message += "HTTP/1.1 ";
    message += std::to_string(statusCode_);
    message += " ";
    message += statusMessage_;
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
