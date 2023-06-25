#include <assert.h>

#include "marco/marco.h"

marco::Logger::ptr g_logger = MARCO_LOG_ROOT();

void test_assert() {
    MARCO_LOG_INFO(g_logger) << marco::BacktraceToString(10);
    MARCO_ASSERT2(0 == 1, "abcdef xx");
}

int main() {
    test_assert();
    return 0;
}