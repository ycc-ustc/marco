#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "config.h"

namespace marco {

const char* LogLevel::ToString(LogLevel::Level level) {
    switch (level) {
#define XX(name)                                                                                   \
    case LogLevel::name:                                                                           \
        return #name;                                                                              \
        break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
    default:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}
LogLevel::Level LogLevel::FromString(const std::string str) {
#define XX(level, v)                                                                               \
    if (str == #v) {                                                                               \
        return LogLevel::level;                                                                    \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOWN;
#undef XX
}
LogEventWrapper::LogEventWrapper(LogEvent::ptr event) : m_event(event) {}
LogEventWrapper::~LogEventWrapper() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrapper::getSS() {
    return m_event->getSS();
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}
void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int   len = vasprintf(&buf, fmt, al);
    if (len != -1) {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

Logger::Logger(const std::string& name) : m_name(name), m_level(LogLevel::DEBUG) {
    m_formatter.reset(
        new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
    for (auto& i : m_appenders) {
        if (!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}
void Logger::setFormatter(const std::string& val) {
    std::cout << "---" << val << std::endl;
    marco::LogFormatter::ptr new_val(new marco::LogFormatter(val));
    if (new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name << " value=" << val
                  << " invalid formatter" << std::endl;
        return;
    }
    // m_formatter = new_val;
    setFormatter(new_val);
}
LogFormatter::ptr Logger::getFormatter() {
    return m_formatter;
}

std::string Logger::toYamlString() {
    YAML::Node node;
    node["name"] = m_name;
    if (m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for (auto& i : m_appenders) {
        // std::cout << "size: " << m_appenders.size() << std::endl;
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file,
                   int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id,
                   uint64_t time)
    : m_file(file),
      m_line(line),
      m_threadId(thread_id),
      m_fiberId(fiber_id),
      m_time(time),
      m_logger(std::move(logger)),
      m_level(level) {}

void Logger::addAppender(LogAppender::ptr appender) {
    if (!appender->getFormatter()) {
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}
void Logger::delAppender(LogAppender::ptr appender) {
    for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
        if (*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders() {
    m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (level >= m_level) {
        auto self = shared_from_this();
        if (!m_appenders.empty()) {
            for (auto& i : m_appenders) {
                i->log(self, level, event);
            }
        } else if (m_root) {
            m_root->log(level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    m_formatter = val;
    if (m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

FileLogAppender::FileLogAppender(const std::string& filename) : m_filename(filename) {
    reopen();
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

std::string FileLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if (m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                            LogEvent::ptr event) {
    if (level >= m_level) {
        std::cout << m_formatter->format(logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString() {
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if (m_level != LogLevel::UNKNOWN) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if (m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {
    if (level >= m_level) {  // level等级大于m_level才会输出，默认m_level == DEBUG
        m_filestream << m_formatter->format(logger, level, event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                                 LogEvent::ptr event) {
    std::stringstream ss;
    for (auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string format = "%Y:%m:%d %H:%M:%S") : m_format(format) {
        if (m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        struct tm tm {};
        time_t    time = event->getTime();  // 得到的是timestamp
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << std::endl;
    }
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << "\t";
    }

private:
    std::string m_string;
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str) : m_string(str) {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
                LogEvent::ptr event) override {
        os << m_string;
    }

private:
    std::string m_string;
};

//%xxx %xxx{xxx} %%
void LogFormatter::init() {
    // str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string                                            nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if ((i + 1) < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int    fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < m_pattern.size()) {
            if (!fmt_status &&
                (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    // std::cout << "*" << str << std::endl;
                    fmt_status = 1;  //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    // std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == m_pattern.size()) {
                if (str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i)
                      << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>>
        s_format_items = {
#define XX(str, C)                                                                                 \
    {                                                                                              \
#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); }                   \
    }

            XX(m, MessageFormatItem),     // m:消息
            XX(p, LevelFormatItem),       // p:日志级别
            XX(r, ElapseFormatItem),      // r:累计毫秒数
            XX(c, NameFormatItem),        // c:日志名称
            XX(t, ThreadIdFormatItem),    // t:线程id
            XX(n, NewLineFormatItem),     // n:换行
            XX(d, DateTimeFormatItem),    // d:时间
            XX(f, FilenameFormatItem),    // f:文件名
            XX(l, LineFormatItem),        // l:行号
            XX(T, TabFormatItem),         // T:Tab
            XX(F, FiberIdFormatItem),     // F:协程id
            XX(N, ThreadNameFormatItem),  // N:线程名称
#undef XX
        };

    for (auto& i : vec) {
        if (std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(
                    new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i)
        //           << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    // std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    // std::cout << "init LoggerManager" << std::endl;
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->m_name] = m_root;
    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) {
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString() {
    YAML::Node node;
    for (auto& i : m_loggers) {
        // std::cout << "logger name: " << i.first <<  std::endl;
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

struct LogAppenderDefine {
    int             type = 0;  // 1 File 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOWN;
    std::string     formatter;
    std::string     file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type && level == oth.level && formatter == oth.formatter &&
               file == oth.file;
    }
};

struct LogDefine {
    std::string                    name;
    LogLevel::Level                level = LogLevel::UNKNOWN;
    std::string                    formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name && level == oth.level && formatter == oth.formatter &&
               appenders == appenders;
    }
    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

template <>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        LogDefine  ld;
        if (!node["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << node << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = node["name"].as<std::string>();
        ld.level =
            LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
        if (node["formatter"].IsDefined()) {
            std::cout << node["formatter"].as<std::string>() << std::endl;
            ld.formatter = node["formatter"].as<std::string>();
        }

        if (node["appenders"].IsDefined()) {
            // std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for (size_t x = 0; x < node["appenders"].size(); ++x) {
                auto a = node["appenders"][x];
                if (!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a << std::endl;
                    continue;
                }
                std::string       type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if (type == "FileLogAppender") {
                    lad.type = 1;
                    if (!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                                  << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if (a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if (type == "StdoutLogAppender") {
                    lad.type = 2;
                    if (a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template <>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if (i.level != LogLevel::UNKNOWN) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if (!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }

        for (auto& a : i.appenders) {
            YAML::Node na;
            if (a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if (a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if (a.level != LogLevel::UNKNOWN) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if (!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

marco::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    marco::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter {
    LogIniter() {
        g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine>& old_value,
                                                const std::set<LogDefine>& new_value) {
            MARCO_LOG_INFO(MARCO_LOG_ROOT()) << "on_logger_conf_changed";

            for (auto& i : new_value) {
                auto it = old_value.find(i);
                std::cout << "name:" << i.name << "  formatter: " << i.formatter << std::endl;
                marco::Logger::ptr logger;
                if (it == old_value.end()) {
                    // 新增logger
                    logger = MARCO_LOG_NAME(i.name);
                } else {
                    if (!(i == *it)) {
                        // 修改的logger
                        logger = MARCO_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if (!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();

                for (auto& a : i.appenders) {
                    marco::LogAppender::ptr ap;
                    if (a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if (a.type == 2) {
                        ap.reset(new StdoutLogAppender);
                    }
                    ap->setLevel(a.level);
                    if (!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if (!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name  << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for (auto& i : old_value) {
                auto it = new_value.find(i);
                if (it == new_value.end()) {
                    // 删除logger
                    auto logger = MARCO_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;

void LoggerManager::init() {}

}  // namespace marco
