#ifndef __MARCO_SOCKET_STREAM_H__
#define __MARCO_SOCKET_STREAM_H__

#include "socket.h"
#include "stream.h"

namespace marco {
class SocketStream : public Stream {
public:
    using ptr = std::shared_ptr<SocketStream>;
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    Socket::ptr getSocket() const {
        return m_socket;
    }
    
    virtual int read(void* buffer, size_t len) override;
    virtual int read(ByteArray::ptr ba, size_t len) override;

    virtual int write(const void* buffer, size_t len) override;
    virtual int write(ByteArray::ptr ba, size_t len) override;

    virtual void close() override;

    bool isConnected() const;

    // Address::ptr getRemoteAddress();

protected:
    Socket::ptr m_socket;
    bool        m_owner;
};
}  // namespace marco

#endif