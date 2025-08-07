#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#endif
namespace fs = std::filesystem;
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() : level_(LogLevel::INFO_) {}

Logger::~Logger() {
    if (ofs_.is_open())
        ofs_.close();
}

void Logger::init(LogLevel level, const std::string& filename) {
#ifdef _WIN32
    // 设置控制台输出为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
    filename_ = filename;

    // 自动创建目录（仅当路径中有/时）
    auto pos = filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string dir = filename.substr(0, pos);
        try {
            fs::create_directories(dir); // 自动递归创建
        } catch (...) {
            std::cerr << "Logger: Failed to create log directory: " << dir << std::endl;
        }
    }

    if (ofs_.is_open()) ofs_.close();
    if (!filename.empty()) {
        ofs_.open(filename, std::ios::app);
        if (!ofs_) {
            std::cerr << "Logger: Failed to open log file: " << filename << std::endl;
        }
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
}

std::string Logger::now() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::log(LogLevel level, const std::string& msg,
                 const char* file, int line) {
    if (level < level_) return;
    std::lock_guard<std::mutex> lock(mtx_);
    std::ostringstream oss;
    oss << "[" << now() << "]"
        << " [" << levelStr_[level] << "] "
        << "(" << file << ":" << line << ") "
        << msg << std::endl;
    if (ofs_.is_open()) ofs_ << oss.str();
    // 同时输出到控制台
    if (level >= LogLevel::ERROR_)
        std::cerr << oss.str();
    else
        std::cout << oss.str();
}
