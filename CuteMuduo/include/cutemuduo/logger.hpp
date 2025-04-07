#pragma once

#include <string_view>
//
#include <cutemuduo/noncopyable.hpp>

namespace cutemuduo {

enum class LogLevel { INFO, WARNING, ERROR, FATAL, DEBUG };

// 单例日志类
class Logger : NonCopyable {
public:
    static Logger &GetInstance();

    void SetLogLevel(LogLevel const &level);

    void Log(std::string_view msg);

private:
    LogLevel log_level_;
};

#define LOG_INFO(format, ...)                                         \
    do {                                                              \
        cutemuduo::Logger &logger = cutemuduo::Logger::GetInstance(); \
        logger.SetLogLevel(cutemuduo::LogLevel::INFO);                \
        char buf[1024] = {0};                                         \
        std::snprintf(buf, 1024, format, ##__VA_ARGS__);              \
        logger.Log(buf);                                              \
    } while (0)

#define LOG_WARNING(format, ...)                                      \
    do {                                                              \
        cutemuduo::Logger &logger = cutemuduo::Logger::GetInstance(); \
        logger.SetLogLevel(cutemuduo::LogLevel::WARNING);             \
        char buf[1024] = {0};                                         \
        std::snprintf(buf, 1024, format, ##__VA_ARGS__);              \
        logger.Log(buf);                                              \
    } while (0)

#define LOG_ERROR(format, ...)                                        \
    do {                                                              \
        cutemuduo::Logger &logger = cutemuduo::Logger::GetInstance(); \
        logger.SetLogLevel(cutemuduo::LogLevel::ERROR);               \
        char buf[1024] = {0};                                         \
        std::snprintf(buf, 1024, format, ##__VA_ARGS__);              \
        logger.Log(buf);                                              \
    } while (0)

#define LOG_FATAL(format, ...)                                        \
    do {                                                              \
        cutemuduo::Logger &logger = cutemuduo::Logger::GetInstance(); \
        logger.SetLogLevel(cutemuduo::LogLevel::FATAL);               \
        char buf[1024] = {0};                                         \
        std::snprintf(buf, 1024, format, ##__VA_ARGS__);              \
        logger.Log(buf);                                              \
        exit(-1);                                                     \
    } while (0)

#ifdef MUDEBUG
#define LOG_DEBUG(format, ...)                                        \
    do {                                                              \
        cutemuduo::Logger &logger = cutemuduo::Logger::GetInstance(); \
        logger.SetLogLevel(cutemuduo::LogLevel::DEBUG);               \
        char buf[1024] = {0};                                         \
        std::snprintf(buf, 1024, format, ##__VA_ARGS__);              \
        logger.Log(buf);                                              \
    } while (0)
#else
#define LOG_DEBUG(format, ...)  // 空定义
#endif

}  // namespace cutemuduo
