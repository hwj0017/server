#pragma once
namespace http
{
enum Method
{
    Invalid,
    Get,
    Post,
    Head,
    Put,
    DeLete,
};
enum Version
{
    Unknow,
    Http10,
    Http11,
};

// end with space
constexpr static auto version_to_string(Version version)
{
    switch (version)
    {
    case Version::Http10:
        return "HTTP/1.0 ";
    case Version::Http11:
        return "HTTP/1.1 ";
    case Version::Unknow:
        return "HTTP/1.1 ";
    }
    return "";
}
enum class StatusCode
{
    Unknow = 0,
    OK = 200,
    MovedPermanently = 301,
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
};

constexpr auto code_to_string(StatusCode code)
{
    switch (code)
    {
    case StatusCode::OK:
        return "200 OK";
    case StatusCode::MovedPermanently:
        return "301 Moved Permanently";
    case StatusCode::BadRequest:
        return "400 Bad Request";
    case StatusCode::Forbidden:
        return "403 Forbidden";
    case StatusCode::NotFound:
        return "404 Not Found";
    case StatusCode::MethodNotAllowed:
        return "405 Method Not Allowed";
    case StatusCode::Unknow:
        return "500 Internal Server Error";
    }
    return "";
}
} // namespace http
