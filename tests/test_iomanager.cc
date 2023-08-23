#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "marco/iomanager.h"
#include "marco/marco.h"

marco::Logger::ptr g_logger = MARCO_LOG_ROOT();
int                sock = 0;

void test_fiber() {
    MARCO_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    // sleep(3);

    // close(sock);
    // marco::IOManager::GetThis()->cancelAll(sock);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "180.101.50.188", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if (errno == EINPROGRESS) {
        MARCO_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        marco::IOManager::GetThis()->addEvent(
            sock, marco::IOManager::READ, []() { MARCO_LOG_INFO(g_logger) << "read callback"; });
        marco::IOManager::GetThis()->addEvent(sock, marco::IOManager::WRITE, []() {
            MARCO_LOG_INFO(g_logger) << "write callback";
            // close(sock);
            marco::IOManager::GetThis()->cancelEvent(sock, marco::IOManager::READ);
            close(sock);
        });
    } else {
        MARCO_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1() {
    std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
    marco::IOManager iom(2);
    iom.schedule(&test_fiber);
}

marco::Timer::ptr s_timer;
void              test_timer() {
    marco::IOManager iom(2);
    s_timer = iom.addTimer(
        1000,
        []() {
            static int i = 0;
            MARCO_LOG_INFO(g_logger) << "hello timer i=" << i;
            if (++i == 3) {
                s_timer->reset(2000, true);
                // s_timer->cancel();
            }
        },
        true);
}

int main(int argc, char** argv) {
    test1();
    // test_timer();
    return 0;
}