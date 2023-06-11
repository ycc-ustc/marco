#include <iostream>
#include <thread>
#include "../macro/log.h"
#include "../macro/util.h"

int main() {
    macro::Logger::ptr logger(new macro::Logger);
    logger->addAppender(
        macro::LogAppender::ptr(new macro::StdoutLogAppender));
    macro::FileLogAppender::ptr file_appender(
        new macro::FileLogAppender("./log.txt"));
    macro::LogFormatter::ptr fmt(new macro::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(macro::LogLevel::ERROR);

    logger->addAppender(file_appender);
    // macro::LogEvent::ptr event(
    // new macro::LogEvent(__FILE__, __LINE__, 0, macro::GetThreadId(),
    // macro::GetFiberId(), time(nullptr)));
    // logger->log(macro::LogLevel::DEBUG, event);
    std::cout << "hello macro log" << std::endl;
    MACRO_LOG_INFO(logger) << "test macro log";
    MACRO_LOG_ERROR(logger) << "test macro error";
    MACRO_LOG_FMT_ERROR(logger, "test macro log error %s", "test");


    auto l = macro::LoggerMgr::GetInstance()->getLogger("xx");
    MACRO_LOG_INFO(l) << "test xxx";
    return 0;
}