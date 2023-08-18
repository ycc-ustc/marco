#include "marco/http/http.h"
#include "marco/log.h"

void test_request() {
    marco::http::HttpRequest::ptr req(new marco::http::HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello marco");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    marco::http::HttpResponse::ptr rsp(new marco::http::HttpResponse);
    rsp->setHeader("X-X", "marco");
    rsp->setBody("hello marco");
    rsp->setStatus((marco::http::HttpStatus)206);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
