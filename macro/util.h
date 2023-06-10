#ifndef __MACRO_UTIL_H__
#define __MACRO_UTIL_H__

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
namespace macro {

pid_t GetThreadId();

u_int32_t GetFiberId();
}  // namespace macro
#endif