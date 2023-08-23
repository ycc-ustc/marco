#include "http_server.h"

// #include "marco/http/servlets/config_servlet.h"
// #include "marco/http/servlets/status_servlet.h"
#include "marco/log.h"

namespace marco::http {

static marco::Logger::ptr g_logger = MARCO_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, marco::IOManager* worker, marco::IOManager* io_worker,
                       marco::IOManager* accept_worker)
    : TcpServer(worker, io_worker, accept_worker), m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);

    // m_type = "http";
    // m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    // m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client) {
    MARCO_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if (!req) {
            MARCO_LOG_DEBUG(g_logger)
                << "recv http request fail, errno=" << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }

        HttpResponse::ptr rsp(
            new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        // rsp->setBody("hello ycy");

        // MARCO_LOG_INFO(g_logger) << "req: " << std::endl << *req;
        // MARCO_LOG_INFO(g_logger) << "rsp: " << std::endl << *rsp;
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);

        if (!m_isKeepalive || req->isClose()) {
            break;
        }
    } while (true);
    session->close();
}

}  // namespace marco::http
