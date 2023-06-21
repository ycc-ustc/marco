#include <iostream>
#include <thread>

#include "marco/log.h"
#include "marco/util.h"

int main() {
    marco::Logger::ptr logger(new marco::Logger);
    logger->addAppender(
        marco::LogAppender::ptr(new marco::StdoutLogAppender));
    marco::FileLogAppender::ptr file_appender(
        new marco::FileLogAppender("./log.txt"));
    marco::LogFormatter::ptr fmt(new marco::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(marco::LogLevel::ERROR);

    logger->addAppender(file_appender);
    // marco::LogEvent::ptr event(
    // new marco::LogEvent(__FILE__, __LINE__, 0, marco::GetThreadId(),
    // marco::GetFiberId(), time(nullptr)));
    // logger->log(marco::LogLevel::DEBUG, event);
    std::cout << "hello marco log" << std::endl;
    MARCO_LOG_INFO(logger) << "test marco log";
    MARCO_LOG_ERROR(logger) << "test marco error";
    MARCO_LOG_FMT_ERROR(logger, "test marco log error %s", "test");

    auto l = marco::LoggerMgr::GetInstance()->getLogger("xx");
    MARCO_LOG_INFO(l) << "test xxx";
    return 0;
}