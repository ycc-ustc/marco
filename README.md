# C++高性能分布式服务器框架
## 1.日志模块
 支持流式日志风格写日志和格式化风格写日志，支持日志格式自定义，日志级别，多日志分离等等功能 流式日志使用：MACRO_LOG_INFO(g_logger) << "this is a log"; 格式化日志使用：MACRO_LOG_FMT_INFO(g_logger, "%s", "this is a log"); 支持时间,线程id,线程名称,日志级别,日志名称,文件名,行号等内容的自由配置