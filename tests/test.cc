#include <iostream>
#include <thread>
#include "../macro/log.h"
#include "../macro/util.h"

int main() {
    macro::Logger::ptr logger(new macro::Logger);
    logger->addAppender(
        macro::LogAppender::ptr(new macro::StdoutLogAppender));
    // macro::LogEvent::ptr event(
    // new macro::LogEvent(__FILE__, __LINE__, 0, macro::GetThreadId(),
    // macro::GetFiberId(), time(nullptr)));
    // logger->log(macro::LogLevel::DEBUG, event);
    std::cout << "hello macro log" << std::endl;
    MACRO_LOG_INFO(logger) << "test macro log";
    return 0;
}