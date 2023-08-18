#include "marco/address.h"
#include "marco/log.h"

marco::Logger::ptr g_logger = MARCO_LOG_ROOT();

void test() {
    std::vector<marco::Address::ptr> addrs;

    MARCO_LOG_INFO(g_logger) << "begin";
    // bool v = marco::Address::Lookup(addrs, "localhost:3080");
    bool v = marco::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    // bool v = marco::Address::Lookup(addrs, "www.sylar.top", AF_INET);
    MARCO_LOG_INFO(g_logger) << "end";
    if (!v) {
        MARCO_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i) {
        MARCO_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = marco::Address::LookupAny("localhost:4080");
    if (addr) {
        MARCO_LOG_INFO(g_logger) << addr->toString();
    } else {
        MARCO_LOG_ERROR(g_logger) << "error";
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<marco::Address::ptr, uint32_t>> results;

    bool v = marco::Address::GetInterfaceAddresses(results);
    if (!v) {
        MARCO_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for (auto& i : results) {
        MARCO_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
                                 << i.second.second;
    }
}

void test_ipv4() {
    // auto addr = marco::IPAddress::Create("www.sylar.top");
    auto addr = marco::IPAddress::Create("127.0.0.8");
    if (addr) {
        MARCO_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_ipv4();
    // test_iface();
    // test();
    return 0;
}
