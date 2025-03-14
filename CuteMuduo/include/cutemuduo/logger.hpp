#pragma once

#include <string_view>
//
#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

enum class LogLevel { INFO, ERROR, FATAL, DEBUG };

// 单例日志类
class Logger : NonCopyable {
public:
    static Logger &GetInstance();

    void SetLogLevel(LogLevel const &level);

    void Log(std::string_view msg);

private:
    LogLevel log_level_;
};

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(format, ...)                       \
    do {                                            \
        Logger &logger = Logger::GetInstance();     \
        logger.SetLogLevel(LogLevel::INFO);         \
        char buf[1024] = {0};                       \
        snprintf(buf, 1024, format, ##__VA_ARGS__); \
        logger.Log(buf);                            \
    } while (0)

#define LOG_ERROR(format, ...)                      \
    do {                                            \
        Logger &logger = Logger::GetInstance();     \
        logger.SetLogLevel(LogLevel::ERROR);        \
        char buf[1024] = {0};                       \
        snprintf(buf, 1024, format, ##__VA_ARGS__); \
        logger.Log(buf);                            \
    } while (0)

#define LOG_FATAL(format, ...)                      \
    do {                                            \
        Logger &logger = Logger::GetInstance();     \
        logger.SetLogLevel(LogLevel::FATAL);        \
        char buf[1024] = {0};                       \
        snprintf(buf, 1024, format, ##__VA_ARGS__); \
        logger.Log(buf);                            \
        exit(-1);                                   \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(format, ...)                      \
    do {                                            \
        Logger &logger = Logger::GetInstance();     \
        logger.SetLogLevel(LoLogLevel::DEBUG);      \
        char buf[1024] = {0};                       \
        snprintf(buf, 1024, format, ##__VA_ARGS__); \
        logger.Log(buf);                            \
    } while (0)
#else
#define LOG_DEBUG(format, ...)
#endif

}  // namespace cutemuduo
