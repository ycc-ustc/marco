#ifndef __MARCO_BYTEARRAY_H__
#define __MARCO_BYTEARRAY_H__
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <string>
#include <vector>

namespace marco {
class ByteArray {
public:
    using ptr = std::shared_ptr<ByteArray>;

    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char*  ptr;
        Node*  next;
        size_t size;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    // write
    void writeFint8(const int8_t val);
    void writeFuint8(const uint8_t val);
    void writeFint16(const int16_t val);
    void writeFuint16(const uint16_t val);
    void writeFint32(const int32_t val);
    void writeFuint32(const uint32_t val);
    void writeFint64(const int64_t val);
    void writeFuint64(const uint64_t val);

    void writeInt32(const int32_t val);
    void writeUint32(const uint32_t val);
    void writeInt64(const int64_t val);
    void writeUint64(const uint64_t val);

    void writeFloat(const float val);
    void writeDouble(const double val);

    // length:int16, data
    void writeStringF16(const std::string& val);
    // length:int32, data
    void writeStringF32(const std::string& val);
    // length:int64, data
    void writeStringF64(const std::string& val);
    // length:varint, data
    void writeStringVint(const std::string& val);
    void writeStringWithoutLength(const std::string& val);

    // read
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float  readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    void clear();

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;

    size_t getPosition() const {
        return m_position;
    }
    void setPosition(size_t val);

    bool writeToFile(const std::string& name) const;

    size_t getBaseSize() const {
        return m_baseSize;
    }
    size_t getReadSize() const {
        return m_size - m_position;
    }

    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    bool readFromFile(const std::string& name);

    std::string toString() const;
    std::string toHexString() const;

    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
    size_t   getSize() const {
        return m_size;
    }

private:
    void   addCapacity(size_t size);
    size_t getCapacity() {
        return m_capacity - m_position;
    }

private:
    size_t m_baseSize;
    size_t m_position;
    size_t m_capacity;
    size_t m_size;
    int8_t m_endian;

    Node* m_root;
    Node* m_cur;
};
}  // namespace marco

#endif
