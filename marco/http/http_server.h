#ifndef __MARCO_HTTP_SERVER_H__
#define __MARCO_HTTP_SERVER_H__

#include "http_session.h"
#include "marco/tcp_server.h"
#include "servlet.h"

namespace marco::http {
class HttpServer : public TcpServer {
public:
    using ptr = std::shared_ptr<HttpServer>;
    HttpServer(bool keepalive = false, marco::IOManager* worker = marco::IOManager::GetThis(),
               marco::IOManager* io_worker = marco::IOManager::GetThis(),
               marco::IOManager* accept_worker = marco::IOManager::GetThis());
    
    ServletDispatch::ptr getServletDispatch() const {
        return m_dispatch;
    }
    void setServletDispatch(ServletDispatch::ptr v) {
        m_dispatch = v;
    }

    virtual void setName(const std::string& v) ;

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;
};

}  // namespace marco::http
#endif
