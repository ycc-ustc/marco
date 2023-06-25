#ifndef __MARCO_LOG_H__
#define __MARCO_LOG_H__

#include <cstdarg>
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "mutex.h"
#include "singleton.h"
#include "util.h"

#define MARCO_LOG_LEVEL(logger, level)                                                             \
    if (logger->getLevel() <= level)                                                               \
    marco::LogEventWrapper(marco::LogEvent::ptr(new marco::LogEvent(                               \
                               logger, level, __FILE__, __LINE__, 0, marco::GetThreadId(),         \
                               marco::GetFiberId(), time(nullptr))))                               \
        .getSS()

#define MARCO_LOG_DEBUG(logger) MARCO_LOG_LEVEL(logger, marco::LogLevel::DEBUG)
#define MARCO_LOG_INFO(logger) MARCO_LOG_LEVEL(logger, marco::LogLevel::INFO)
#define MARCO_LOG_WARN(logger) MARCO_LOG_LEVEL(logger, marco::LogLevel::WARN)
#define MARCO_LOG_ERROR(logger) MARCO_LOG_LEVEL(logger, marco::LogLevel::ERROR)
#define MARCO_LOG_FATAL(logger) MARCO_LOG_LEVEL(logger, marco::LogLevel::FATAL)

#define MARCO_LOG_FMT_LEVEL(logger, level, fmt, ...)                                               \
    if (logger->getLevel() <= level)                                                               \
    marco::LogEventWrapper(marco::LogEvent::ptr(new marco::LogEvent(                               \
                               logger, level, __FILE__, __LINE__, 0, marco::GetThreadId(),         \
                               marco::GetFiberId(), time(nullptr))))                               \
        .getEvent()                                                                                \
        ->format(fmt, __VA_ARGS__)

#define MARCO_LOG_FMT_DEBUG(logger, fmt, ...)                                                      \
    MARCO_LOG_FMT_LEVEL(logger, marco::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define MARCO_LOG_FMT_INFO(logger, fmt, ...)                                                       \
    MARCO_LOG_FMT_LEVEL(logger, marco::LogLevel::INFO, fmt, __VA_ARGS__)
#define MARCO_LOG_FMT_WARN(logger, fmt, ...)                                                       \
    MARCO_LOG_FMT_LEVEL(logger, marco::LogLevel::WARN, fmt, __VA_ARGS__)
#define MARCO_LOG_FMT_ERROR(logger, fmt, ...)                                                      \
    MARCO_LOG_FMT_LEVEL(logger, marco::LogLevel::ERROR, fmt, __VA_ARGS__)
#define MARCO_LOG_FMT_FATAL(logger, fmt, ...)                                                      \
    MARCO_LOG_FMT_LEVEL(logger, marco::LogLevel::FATAL, fmt, __VA_ARGS__)

#define MARCO_LOG_ROOT() marco::LoggerMgr::GetInstance()->getRoot()

#define MARCO_LOG_NAME(name) marco::LoggerMgr::GetInstance()->getLogger(name)

namespace marco {
class Logger;
class LoggerManager;

// 日志级别
class LogLevel {
public:
    enum Level { UNKNOWN = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

    static const char*     ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string str);
};

// 日志事件
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line,
             uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    const char* getFile() const {
        return m_file;
    }
    int32_t getLine() const {
        return m_line;
    }
    uint32_t getElapse() const {
        return m_elapse;
    }
    uint32_t getThreadId() const {
        return m_threadId;
    }
    uint32_t getFiberId() const {
        return m_fiberId;
    }
    uint64_t getTime() const {
        return m_time;
    }
    const std::string getContent() {
        return m_ss.str();
    }
    const std::string& getThreadName() const {
        return m_threadName;
    }
    // 返回日志内容字符串流
    std::stringstream& getSS() {
        return m_ss;
    }
    std::shared_ptr<Logger> getLogger() {
        return m_logger;
    }

    LogLevel::Level getLevel() {
        return m_level;
    }
    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

private:
    const char*             m_file = nullptr;  // 文件名
    int32_t                 m_line = 0;        // 行号
    uint32_t                m_elapse = 0;      // 程序启动开始到现在的毫秒数
    uint32_t                m_threadId = 0;    // 线程id
    uint32_t                m_fiberId = 0;     // 协程id
    uint64_t                m_time;            // 时间戳
    std::string             m_threadName;      // 线程名称
    std::stringstream       m_ss;              // 日志内容流
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level         m_level;
};

class LogEventWrapper {
public:
    LogEventWrapper(LogEvent::ptr event);
    ~LogEventWrapper();
    std::stringstream& getSS();
    LogEvent::ptr      getEvent() {
        return m_event;
    }

private:
    LogEvent::ptr m_event;
};

// 日志格式器
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    void        init();
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level,
                            LogEvent::ptr event) = 0;
    };
    bool isError() const {
        return m_error;
    }
    std::string getPattern() {
        return m_pattern;
    }

private:
    std::string                  m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool                         m_error = false;
};

// 日志输出地
class LogAppender {
    friend class Logger;

public:
    using ptr = std::shared_ptr<LogAppender>;
    using MutexType = Mutex;

    virtual ~LogAppender() {}
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event) = 0;

    virtual std::string toYamlString() = 0;

    void              setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();

    void setLevel(LogLevel::Level level) {
        m_level = level;
    }
    LogLevel::Level getLevel() {
        return m_level;
    }

protected:
    LogLevel::Level   m_level = LogLevel::DEBUG;
    LogFormatter::ptr m_formatter;
    bool              m_hasFormatter = false;
    MutexType         m_mutex;
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

// 输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
    // 文件打开成功 返回true
    bool reopen();

private:
    std::string   m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger> {
    friend class LoggerManager;

public:
    using ptr = std::shared_ptr<Logger>;
    using MutexType = Mutex;
    Logger(const std::string& name = "root");

    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();

    std::string getName() const {
        return m_name;
    }
    LogLevel::Level getLevel() const {
        return m_level;
    };
    void setLevel(LogLevel::Level val) {
        m_level = val;
    }

    void              setFormatter(LogFormatter::ptr val);
    void              setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();

    std::string toYamlString();

private:
    std::string                 m_name;       // 日志名称
    LogLevel::Level             m_level;      // 日志级别
    std::list<LogAppender::ptr> m_appenders;  // appender集合
    LogFormatter::ptr           m_formatter;
    Logger::ptr                 m_root;
    MutexType                   m_mutex;
};
class LoggerManager {
public:
    using MutexType = Mutex;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    std::string toYamlString();

    void init();

    Logger::ptr getRoot() {
        return m_root;
    }

private:
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr                        m_root;
    Mutex                              m_mutex;
};

using LoggerMgr = marco::Singleton<LoggerManager>;
}  // namespace marco
#endif
