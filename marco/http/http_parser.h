#ifndef __MARCO_HTTP_PARSER_H__
#define __MARCO_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"
namespace marco::http {
class HttpRequestParser {
public:
    using ptr = std::shared_ptr<HttpRequestParser>;
    HttpRequestParser();
    size_t          execute(char* data, size_t len);
    int             isFinished();
    int             hasError();
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();

    uint64_t getContentLength();

    HttpRequest::ptr getData() const {
        return m_data;
    }
    void setError(int err) {
        m_error = err;
    }

private:
    http_parser      m_parser;
    HttpRequest::ptr m_request;
    int              m_error;
    HttpRequest::ptr m_data;
};

class HttpResponseParser {
public:
    using ptr = std::shared_ptr<HttpResponseParser>;

    HttpResponseParser();
    size_t execute(char* data, size_t len, bool chunck);
    int    isFinished();
    int    hasError();

    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();

    uint64_t          getContentLength();
    HttpResponse::ptr getData() const {
        return m_data;
    }
    void setError(int err) {
        m_error = err;
    }

    httpclient_parser& getParser() {
        return m_parser;
    }

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_request;
    /// 1001: invalid version
    /// 1002: invalid field
    int               m_error;
    HttpResponse::ptr m_data;
};
}  // namespace marco::http
#endif