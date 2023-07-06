#ifndef __MARCO_UTIL_H__
#define __MARCO_UTIL_H__

#include <cxxabi.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
namespace marco {

template <class T>
const char* TypeToName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}

pid_t GetThreadId();

uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

uint64_t GetCurrentMS();

uint64_t GetCurrentUS();

}  // namespace marco
#endif