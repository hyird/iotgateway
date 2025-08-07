#pragma once
#include <fstream>
#include <string>
#include <mutex>
#include <map>

enum class LogLevel { DEBUG_, INFO_, WARN_, ERROR_, FATAL_ };

class Logger {
public:
    static Logger& instance();
    void init(LogLevel level, const std::string& filename);
    void log(LogLevel level, const std::string& msg, const char* file, int line);
    void setLevel(LogLevel level);

private:
    Logger();
    ~Logger();
    std::ofstream ofs_;
    std::mutex mtx_;
    LogLevel level_;
    std::string filename_;
    std::map<LogLevel, std::string> levelStr_ = {
        {LogLevel::DEBUG_, "DEBUG"},
        {LogLevel::INFO_,  "INFO"},
        {LogLevel::WARN_,  "WARN"},
        {LogLevel::ERROR_, "ERROR"},
        {LogLevel::FATAL_, "FATAL"}
    };
    std::string now();
};

// 日志宏
#define GLOG_DEBUG(msg) Logger::instance().log(LogLevel::DEBUG_, msg, __FILE__, __LINE__)
#define GLOG_INFO(msg)  Logger::instance().log(LogLevel::INFO_,  msg, __FILE__, __LINE__)
#define GLOG_WARN(msg)  Logger::instance().log(LogLevel::WARN_,  msg, __FILE__, __LINE__)
#define GLOG_ERROR(msg) Logger::instance().log(LogLevel::ERROR_, msg, __FILE__, __LINE__)
#define GLOG_FATAL(msg) Logger::instance().log(LogLevel::FATAL_, msg, __FILE__, __LINE__)
