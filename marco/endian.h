#ifndef __MARCO_ENDIAN_H__
#define __MARCO_ENDIAN_H__

#include <type_traits>
#define MARCO_LITTLE_ENDIAN 1
#define MARCO_BIG_ENDIAN 2

#include <byteswap.h>

#include <cstdint>

namespace marco {

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(T val) {
    return (T)bswap_64((uint64_t)val);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(T val) {
    return (T)bswap_32((uint32_t)val);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(T val) {
    return (T)bswap_16((uint16_t)val);
}

// BYTE_ORDER 是一个宏常量，用于表示当前系统的字节序
#if BYTE_ORDER == BIG_ENDIAN 
#define MARCO_BYTE_ORDER MARCO_BIG_ENDIAN
#else
#define MARCO_BYTE_ORDER MARCO_LITTLE_ENDIAN
#endif

#if MARCO_BYTE_ORDER == MARCO_BIG_ENDIAN

template <class T>
T byteswapOnLittleEndian(T t) {
    return t;
}


template <class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else

template <class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

template <class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

}  // namespace marco

#endif
