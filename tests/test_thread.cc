#include "marco/marco.h"

marco::Logger::ptr g_logger = MARCO_LOG_ROOT();
// marco::RWMutex       s_mutex;
int count = 0;

void fun1() {
    MARCO_LOG_INFO(g_logger) << "name: " << marco::Thread::GetName()
                             << " this.name: " << marco::Thread::GetThis()->getName()
                             << " id: " << marco::GetThreadId()
                             << " this.id: " << marco::Thread::GetThis()->getId();
    for (int i = 0; i < 100000; ++i) {
        // marco::RWMutex::WriteLock lock(s_mutex);
        // marco::Mutex::Lock lock(s_mutex);
        count++;
    }
}

void fun2() {
    while (true) {
        MARCO_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

int main(int argc, char** argv) {
    MARCO_LOG_INFO(g_logger) << "thread test begin";

    // YAML::Node root = YAML::LoadFile("/home/dev/marco/bin/config/log2.yaml");
    // marco::Config::LoadFromYaml(root);

    std::vector<marco::Thread::ptr> thrs;
    for (int i = 0; i < 10; i++) {
        marco::Thread::ptr thr(new marco::Thread(&fun1, "name_" + std::to_string(i* 2)));
        // marco::Thread::ptr thr2(new marco::Thread(&fun2, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        // thrs.push_back(thr2);
    }
    // std::cout << "......." << std::endl;
    for (int i = 0; i < 10; i++) {
        // std::cout << "i: " << i << std::endl;
        thrs[i]->join();
    }

    MARCO_LOG_INFO(g_logger) << "thread test end";
    MARCO_LOG_INFO(g_logger) << "count=" << count;
    return 0;
}