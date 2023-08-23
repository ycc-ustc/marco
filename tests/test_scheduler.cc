#include "marco/marco.h"
#include "marco/hook.h"

static marco::Logger::ptr g_logger = MARCO_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    MARCO_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if (--s_count >= 0) {
        marco::Scheduler::GetThis()->schedule(&test_fiber);
    }
}

int main(int argc, char** argv) {
    marco::Thread::SetName("main");
    marco::Scheduler sc(3, true, "test");
    MARCO_LOG_INFO(g_logger) << "main";
    // marco::Scheduler sc;

    sc.start();
    sleep(2);
    MARCO_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    MARCO_LOG_INFO(g_logger) << "over";
    return 0;
}
