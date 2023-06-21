# C++高性能分布式服务器框架
## 1.日志模块
 支持流式日志风格写日志和格式化风格写日志，支持日志格式自定义，日志级别，多日志分离等等功能 流式日志使用：MARCO_LOG_INFO(g_logger) << "this is a log"; 格式化日志使用：MARCO_LOG_FMT_INFO(g_logger, "%s", "this is a log"); 支持时间,线程id,线程名称,日志级别,日志名称,文件名,行号等内容的自由配置
## 2.配置系统
采用约定由于配置的思想。定义即可使用。不需要单独去解析。支持变更通知功能。使用YAML文件做为配置内容。支持级别格式的数据类型，支持STL容器(vector,list,set,map等等),支持自定义类型的支持（需要实现序列化和反序列化方法)使用方式如下：
```cpp
static marco::ConfigVar<int>::ptr g_tcp_connect_timeout =
	marco::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");
```
定义了一个tcp连接超时参数，可以直接使用 g_tcp_connect_timeout->getValue() 获取参数的值，当配置修改重新加载，该值自动更新
上述配置格式如下：
```sh
tcp:
    connect:
            timeout: 10000
```