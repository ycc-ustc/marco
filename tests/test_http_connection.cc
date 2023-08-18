#include <fstream>
#include <iostream>

#include "marco/http/http_connection.h"
#include "marco/http/http_parser.h"
#include "marco/iomanager.h"
#include "marco/log.h"
// #include "marco/streams/zlib_stream.h"

static marco::Logger::ptr g_logger = MARCO_LOG_ROOT();

void test_pool() {
    marco::http::HttpConnectionPool::ptr pool(
        new marco::http::HttpConnectionPool("www.sylar.top", "", 80, false, 10, 1000 * 30, 5));

    marco::IOManager::GetThis()->addTimer(
        1000,
        [pool]() {
            auto r = pool->doGet("/", 300);
            MARCO_LOG_INFO(g_logger) << r->toString();
        },
        true);
}

void run() {
    marco::Address::ptr addr = marco::Address::LookupAnyIPAddress("www.sylar.top:80");
    if (!addr) {
        MARCO_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    marco::Socket::ptr sock = marco::Socket::CreateTCP(addr);
    bool               rt = sock->connect(addr);
    if (!rt) {
        MARCO_LOG_INFO(g_logger) << "connect " << (*addr).toString() << " failed";
        return;
    }

    marco::http::HttpConnection::ptr conn(new marco::http::HttpConnection(sock));
    marco::http::HttpRequest::ptr    req(new marco::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    MARCO_LOG_INFO(g_logger) << "req:" << std::endl << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        MARCO_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    MARCO_LOG_INFO(g_logger) << "rsp:" << std::endl << *rsp;

    std::ofstream ofs("rsp.dat");
    ofs << *rsp;

    MARCO_LOG_INFO(g_logger) << "=========================";

    auto r = marco::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 3000);
    MARCO_LOG_INFO(g_logger) << "result=" << r->result << " error=" << r->error
                             << " rsp=" << (r->response ? r->response->toString() : "");

    MARCO_LOG_INFO(g_logger) << "=========================";
    test_pool();
}

// void test_https() {
//     auto r = marco::http::HttpConnection::DoGet("http://www.baidu.com/", 300,
//                                                 {{"Accept-Encoding", "gzip, deflate, br"},
//                                                  {"Connection", "keep-alive"},
//                                                  {"User-Agent", "curl/7.29.0"}});
//     MARCO_LOG_INFO(g_logger) << "result=" << r->result << " error=" << r->error
//                              << " rsp=" << (r->response ? r->response->toString() : "");

//     // marco::http::HttpConnectionPool::ptr pool(new marco::http::HttpConnectionPool(
//     //            "www.baidu.com", "", 80, false, 10, 1000 * 30, 5));
//     auto pool =
//         marco::http::HttpConnectionPool::Create("https://www.baidu.com", "", 10, 1000 * 30, 5);
//     marco::IOManager::GetThis()->addTimer(
//         1000,
//         [pool]() {
//             auto r = pool->doGet(
//                 "/", 3000,
//                 {{"Accept-Encoding", "gzip, deflate, br"}, {"User-Agent", "curl/7.29.0"}});
//             MARCO_LOG_INFO(g_logger) << r->toString();
//         },
//         true);
// }

// void test_data() {
//     marco::Address::ptr addr = marco::Address::LookupAny("www.baidu.com:80");
//     auto                sock = marco::Socket::CreateTCP(addr);

//     sock->connect(addr);
//     const char buff[] = "GET / HTTP/1.1\r\n"
//                         "connection: close\r\n"
//                         "Accept-Encoding: gzip, deflate, br\r\n"
//                         "Host: www.baidu.com\r\n\r\n";
//     sock->send(buff, sizeof(buff));

//     std::string line;
//     line.resize(1024);

//     std::ofstream ofs("http.dat", std::ios::binary);
//     int           total = 0;
//     int           len = 0;
//     while ((len = sock->recv(&line[0], line.size())) > 0) {
//         total += len;
//         ofs.write(line.c_str(), len);
//     }
//     std::cout << "total: " << total << " tellp=" << ofs.tellp() << std::endl;
//     ofs.flush();
// }

// void test_parser() {
//     std::ifstream ifs("http.dat", std::ios::binary);
//     std::string   content;
//     std::string   line;
//     line.resize(1024);

//     int total = 0;
//     while (!ifs.eof()) {
//         ifs.read(&line[0], line.size());
//         content.append(&line[0], ifs.gcount());
//         total += ifs.gcount();
//     }

//     std::cout << "length: " << content.size() << " total: " << total << std::endl;
//     marco::http::HttpResponseParser parser;
//     size_t                          nparse = parser.execute(&content[0], content.size(), false);
//     std::cout << "finish: " << parser.isFinished() << std::endl;
//     content.resize(content.size() - nparse);
//     std::cout << "rsp: " << *parser.getData() << std::endl;

//     auto&       client_parser = parser.getParser();
//     std::string body;
//     int         cl = 0;
//     do {
//         size_t nparse = parser.execute(&content[0], content.size(), true);
//         std::cout << "content_len: " << client_parser.content_len << " left: " << content.size()
//                   << std::endl;
//         cl += client_parser.content_len;
//         content.resize(content.size() - nparse);
//         body.append(content.c_str(), client_parser.content_len);
//         content = content.substr(client_parser.content_len + 2);
//     } while (!client_parser.chunks_done);

//     std::cout << "total: " << body.size() << " content:" << cl << std::endl;

//     marco::ZlibStream::ptr stream = marco::ZlibStream::CreateGzip(false);
//     stream->write(body.c_str(), body.size());
//     stream->flush();

//     body = stream->getResult();

//     std::ofstream ofs("http.txt");
//     ofs << body;
// }

int main(int argc, char** argv) {
    marco::IOManager iom(2);
    iom.schedule(run);
    // iom.schedule(test_https);
    return 0;
}
