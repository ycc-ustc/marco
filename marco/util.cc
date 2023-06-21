#include "util.h"

namespace marco {
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

u_int32_t GetFiberId() {
    return 0;
}
}  // namespace marco