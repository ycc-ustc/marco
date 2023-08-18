#ifndef __MARCO_STREAM_H__
#define __MARCO_STREAM_H__

#include <memory>

#include "bytearray.h"

namespace marco {
class Stream {
public:
    using ptr = std::shared_ptr<Stream>;
    virtual ~Stream() {}

    virtual int read(void* buffer, size_t len) = 0;
    virtual int read(ByteArray::ptr ba, size_t len) = 0;
    virtual int readFixSize(void* buffer, size_t len);
    virtual int readFixSize(ByteArray::ptr ba, size_t len);

    virtual int write(const void* buffer, size_t len) = 0;
    virtual int write(ByteArray::ptr ba, size_t len) = 0;
    virtual int writeFixSize(const void* buffer, size_t len);
    virtual int writeFixSize(ByteArray::ptr ba, size_t len);

    virtual void close() = 0;
};
}  // namespace marco

#endif