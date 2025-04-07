#include <iostream>
//
#include <cutemuduo/logger.hpp>
#include <cutemuduo/timestamp.hpp>

namespace cutemuduo {

Logger& Logger::GetInstance() {
    // NOTE: C++11 函数内的静态变量会在第一次访问时初始化，并且保证线程安全
    static Logger ins;
    return ins;
}

void Logger::SetLogLevel(LogLevel const& level) { log_level_ = level; }

void Logger::Log(std::string_view msg) {
    std::string pre = "";
    switch (log_level_) {
        case LogLevel::INFO:
            pre = "[INFO]";
            break;
        case LogLevel::WARNING:
            pre = "[WARNING]";
            break;
        case LogLevel::ERROR:
            pre = "[ERROR]";
            break;
        case LogLevel::FATAL:
            pre = "[FATAL]";
            break;
        case LogLevel::DEBUG:
            pre = "[DEBUG]";
            break;
        default:
            break;
    }

    // 打印时间和msg
    std::cout << pre + Timestamp::Now().ToString() << " : " << msg << std::endl;
}

}  // namespace cutemuduo
