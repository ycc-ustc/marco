#include "marco/iomanager.h"
#include "marco/socket.h"
#include "marco/marco.h"

static marco::Logger::ptr g_looger = MARCO_LOG_ROOT();

void test_socket() {
    // std::vector<marco::Address::ptr> addrs;
    // marco::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // marco::IPAddress::ptr addr;
    // for(auto& i : addrs) {
    //    MARCO_LOG_INFO(g_looger) << i->toString();
    //    addr = std::dynamic_pointer_cast<marco::IPAddress>(i);
    //    if(addr) {
    //        break;
    //    }
    //}
    marco::IPAddress::ptr addr = marco::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr) {
        MARCO_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        MARCO_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    marco::Socket::ptr sock = marco::Socket::CreateTCP(addr);
    addr->setPort(80 );
    MARCO_LOG_INFO(g_looger) << "addr=" << addr->toString();
    if (!sock->connect(addr)) {
        MARCO_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        MARCO_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int        rt = sock->send(buff, sizeof(buff));
    if (rt <= 0) {
        MARCO_LOG_INFO(g_looger) << "send fail rt=" << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if (rt <= 0) {
        MARCO_LOG_INFO(g_looger) << "recv fail rt=" << rt;
        return;
    }

    buffs.resize(rt);
    MARCO_LOG_INFO(g_looger) << buffs;
}

void test2() {
    marco::IPAddress::ptr addr = marco::Address::LookupAnyIPAddress("www.baidu.com:80");
    if (addr) {
        MARCO_LOG_INFO(g_looger) << "get address: " << addr->toString();
    } else {
        MARCO_LOG_ERROR(g_looger) << "get address fail";
        return;
    }

    marco::Socket::ptr sock = marco::Socket::CreateTCP(addr);
    if (!sock->connect(addr)) {
        MARCO_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        MARCO_LOG_INFO(g_looger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = marco::GetCurrentUS();
    for (size_t i = 0; i < 20000000ul; ++i) {
        if (int err = sock->getError()) {
            MARCO_LOG_INFO(g_looger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        // struct tcp_info tcp_info;
        // if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    MARCO_LOG_INFO(g_looger) << "err";
        //    break;
        //}
        // if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    MARCO_LOG_INFO(g_looger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if (i && (i % batch) == 0) {
            uint64_t ts2 = marco::GetCurrentUS();
            MARCO_LOG_INFO(g_looger)
                << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main(int argc, char** argv) {
    marco::IOManager iom;
    // iom.schedule(&test_socket);
    iom.schedule(&test2);
    return 0;
}
