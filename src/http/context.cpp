#include "context.h"
#include <string_view>

namespace http
{
// 解析头行
bool Context::processRequestLine(std::string_view line)
{
    bool succeed = false;
    auto start = line.begin();
    auto end = line.end();
    auto space = std::find(start, end, ' ');
    if (space != end && request_.setMethod({start, space}))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            auto question = std::find(start, space, '?');
            if (question != space)
            {
                request_.setPath({start, question});
                request_.setQuery({question, space});
            }
            else
            {
                request_.setPath({start, space});
            }
            start = space + 1;
            succeed = (end - start == 8) && std::equal(start, end - 1, "HTTP/1.");
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion(Request::Http11);
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion(Request::Http10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

bool Context::parseRequest(std::string_view buf)
{
    static char CRLF[] = "\r\n";
    bool ok = true;
    bool hasMore = true;
    auto begin = buf.begin();
    auto end = buf.end();
    auto now = begin;
    auto receiveTime = utils::TimeSpec::getNow();

    while (hasMore)
    {
        if (state_ == ExpectRequestLine)
        {
            auto crlf = std::search(begin, end, CRLF, CRLF + 2);
            if (crlf != end)
            {
                ok = processRequestLine({begin, crlf});
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    state_ = ExpectHeaders;
                    now = crlf + 2;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == ExpectHeaders)
        {
            auto crlf = std::search(now, end, CRLF, CRLF + 2);
            if (crlf != end)
            {
                if (crlf != now)
                {
                    auto colon = std::find(now, crlf, ':');
                    if (colon != crlf)
                    {
                        auto field = std::string_view(now, colon);

                        // skip space
                        auto value_begin = colon + 1;
                        auto value_end = crlf;
                        while (value_begin < crlf && isspace(*value_begin))
                        {
                            ++value_begin;
                        }
                        while (value_end > value_begin && isspace(*(value_end - 1)))
                        {
                            --value_end;
                        }
                        request_.addHeader(field, {value_begin, value_end});
                        now = crlf + 2;
                    }
                }
                // 空行
                else
                {
                    state_ = ExpectBody;
                    now = crlf + 2;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        else if (state_ == ExpectBody)
        {
            request_.setBody({now, end});
            state_ = GotAll;
            hasMore = false;
        }
    }
    return ok;
}

} // namespace http
