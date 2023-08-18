#include "marco/iomanager.h"
#include "marco/log.h"
#include "marco/tcp_server.h"

marco::Logger::ptr g_logger = MARCO_LOG_ROOT();

void run() {
    auto addr = marco::Address::LookupAny("0.0.0.0:8033");
    // auto addr2 = marco::UnixAddress::ptr(new marco::UnixAddress("/tmp/unix_addr"));
    std::vector<marco::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    marco::TcpServer::ptr            tcp_server(new marco::TcpServer);
    std::vector<marco::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}
int main(int argc, char** argv) {
    marco::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
