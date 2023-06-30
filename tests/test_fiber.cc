#include "marco/marco.h"

marco::Logger::ptr g_logger = MARCO_LOG_NAME("root");

void run_in_fiber() {
    MARCO_LOG_INFO(g_logger) << "run_in_fiber begin";
    marco::Fiber::YieldToHold();
    MARCO_LOG_INFO(g_logger) << "run_in_fiber end";
    marco::Fiber::YieldToHold();
}

void test_fiber() {
    MARCO_LOG_INFO(g_logger) << "main begin -1";
    {
        marco::Fiber::GetThis();
        MARCO_LOG_INFO(g_logger) << "main begin";
        marco::Fiber::ptr fiber(new marco::Fiber(run_in_fiber));
        fiber->swapIn();
        MARCO_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        MARCO_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    MARCO_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    marco::Thread::SetName("main");

    std::vector<marco::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i) {
        thrs.push_back(
            marco::Thread::ptr(new marco::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for (auto i : thrs) {
        i->join();
    }
    // test_fiber();
    return 0;
}