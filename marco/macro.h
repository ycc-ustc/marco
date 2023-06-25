#ifndef __MARCO_MACRO_H__
#define __MARCO_MACRO_H__

#include <cassert>
#include <cstring>

#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKELY 宏的封装, 告诉编译器优化,条件大概率成立
#define MARCO_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKELY 宏的封装, 告诉编译器优化,条件大概率不成立
#define MARCO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MARCO_LIKELY(x) (x)
#define MARCO_UNLIKELY(x) (x)
#endif

/// 断言宏封装
#define MARCO_ASSERT(x)                                                                            \
    if (MARCO_UNLIKELY(!(x))) {                                                                    \
        MARCO_LOG_ERROR(MARCO_LOG_ROOT()) << "ASSERTION: " #x << "\nbacktrace:\n"                  \
                                          << marco::BacktraceToString(100, 2, "    ");             \
        assert(x);                                                                                 \
    }

/// 断言宏封装
#define MARCO_ASSERT2(x, w)                                                                        \
    if (MARCO_UNLIKELY(!(x))) {                                                                    \
        MARCO_LOG_ERROR(MARCO_LOG_ROOT()) << "ASSERTION: " #x << "\n"                              \
                                          << w << "\nbacktrace:\n"                                 \
                                          << marco::BacktraceToString(100, 2, "    ");             \
        assert(x);                                                                                 \
    }

#endif