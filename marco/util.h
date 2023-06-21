#ifndef __MARCO_UTIL_H__
#define __MARCO_UTIL_H__

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
namespace marco {

pid_t GetThreadId();

u_int32_t GetFiberId();
}  // namespace marco
#endif