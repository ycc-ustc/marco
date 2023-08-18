#ifndef __MARCO_HTTP_SESSION_H__
#define __MARCO_HTTP_SESSION_H__

#include "../socket_stream.h"
#include "http.h"

namespace marco::http {
class HttpSession : public SocketStream {
public:
    using ptr = std::shared_ptr<HttpSession>;
    HttpSession(Socket::ptr sock, bool owner=true);

    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);
};
}  // namespace marco

#endif
