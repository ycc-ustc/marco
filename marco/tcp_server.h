#ifndef __MARCO_TCP_SERVER_H__
#define __MARCO_TCP_SERVER_H__

#include <functional>
#include <memory>
#include <vector>

#include "iomanager.h"
#include "socket.h"

namespace marco {
class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    using ptr = std::shared_ptr<TcpServer>;
    TcpServer(marco::IOManager* worker = marco::IOManager::GetThis(),
              marco::IOManager* io_woker = marco::IOManager::GetThis(),
              marco::IOManager* accept_worker = marco::IOManager::GetThis());

    virtual ~TcpServer();
    virtual bool bind(marco::Address::ptr addr, bool ssl = false);
    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails,
                      bool ssl = false);
    virtual bool start();

    virtual void stop();

    void startAccept(Socket::ptr sock);

    // uint64_t getReadTimeout() const {
    //     return m_readTimeout;
    // }
    std::string getName() const {
        return m_name;
    }
    // void setReadTimeout(uint64_t val) {
    // m_readTimeout = val;
    // }
    void setName(const std::string& v) {
        m_name = v;
    }

    bool isStop() const {
        return m_isStop;
    }

    std::string toString(const std::string& prefix);

protected:
    virtual void handleClient(Socket::ptr client);

private:
    std::vector<Socket::ptr> m_socks;
    /// 服务器Socket接收连接的调度器
    IOManager* m_worker;
    IOManager* m_ioWorker;
    IOManager* m_acceptWorker;
    /// 接收超时时间(毫秒)
    uint64_t    m_recvTimeout;
    std::string m_name;
    bool        m_isStop;
    /// 服务器类型
    std::string m_type = "tcp";
    bool        m_ssl = false;
};
}  // namespace marco

#endif
