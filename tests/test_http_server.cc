#include "marco/http/http_server.h"
#include "marco/log.h"

static marco::Logger::ptr g_logger = MARCO_LOG_ROOT();

#define XX(...) #__VA_ARGS__

marco::IOManager::ptr worker;
void                  run() {
    g_logger->setLevel(marco::LogLevel::INFO);
    // marco::http::HttpServer::ptr server(new marco::http::HttpServer(true, worker.get(),
    // marco::IOManager::GetThis()));
    marco::http::HttpServer::ptr server(new marco::http::HttpServer(true));
    marco::Address::ptr          addr = marco::Address::LookupAnyIPAddress("0.0.0.0:5700");
    while (!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/marco/xx",
                   [](marco::http::HttpRequest::ptr req, marco::http::HttpResponse::ptr rsp,
                      marco::http::HttpSession::ptr session) {
                       rsp->setBody(req->toString());
                       return 0;
                   });

    sd->addGlobServlet("/marco/*",
                       [](marco::http::HttpRequest::ptr req, marco::http::HttpResponse::ptr rsp,
                          marco::http::HttpSession::ptr session) {
                           rsp->setBody("Glob:\r\n" + req->toString());
                           return 0;
                       });

    sd->addGlobServlet("/marcox/*", [](marco::http::HttpRequest::ptr  req,
                                       marco::http::HttpResponse::ptr rsp,
                                       marco::http::HttpSession::ptr  session) {
        rsp->setBody(XX(<html><head><title> 404 Not Found</ title></ head><body><center>
                                <h1> 404 Not        Found</ h1></ center><hr><center>
                                                    nginx /
                                1.16.0 <
                            / center > </ body>< / html> < !--a padding to disable MSIE and
                        Chrome friendly error page-- > < !--a padding to disable MSIE and
                        Chrome friendly error page-- > < !--a padding to disable MSIE and
                        Chrome friendly error page-- > < !--a padding to disable MSIE and
                        Chrome friendly error page-- > < !--a padding to disable MSIE and
                        Chrome friendly error page-- > < !--a padding to disable MSIE and
                        Chrome friendly error page-- >));
        return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    marco::IOManager iom(1, true, "main");
    worker.reset(new marco::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
