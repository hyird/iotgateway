#include "Logger.h"
#include "JsonConfig.h"
#include "ThreadPool.h"
#include "TimerScheduler.h"
#include "DeviceManager.h"
#include <iostream>
#include <memory>
#include <thread>

int main(const int argc, char* argv[]) {

    try {
        // 1. 读取配置文件路径
        std::string configPath = "config/iot_config.json";
        if (argc > 1) configPath = argv[1];
        // 2. 解析配置
        GlobalConfig globalConfig;
        try {
            globalConfig = JsonConfig::load(configPath);
        } catch (const std::exception& ex) {
            std::cerr << "Failed to parse config: " << ex.what() << std::endl;
            return 1;
        }
        // 3. 初始化日志
        auto logLevel = LogLevel::INFO_;
        if (const std::string lvl = globalConfig.system.log_level; lvl == "debug") logLevel = LogLevel::DEBUG_;
        else if (lvl == "info")  logLevel = LogLevel::INFO_;
        else if (lvl == "warn")  logLevel = LogLevel::WARN_;
        else if (lvl == "error") logLevel = LogLevel::ERROR_;
        else if (lvl == "fatal") logLevel = LogLevel::FATAL_;
        Logger::instance().init(logLevel, globalConfig.system.log_file);
        GLOG_INFO("========= IoT Gateway Starting =========");
        // 4. 初始化线程池和定时调度器
        size_t poolSize = std::max(1, globalConfig.system.thread_pool_size);
        auto threadPool = std::make_shared<ThreadPool>(poolSize);
        const auto timerScheduler = std::make_shared<TimerScheduler>(threadPool);
        // 5. 初始化设备管理器，加载所有设备/分组/变量
        const auto deviceManager = DeviceManager::create(threadPool, timerScheduler, globalConfig);
        // // 6. 注册所有采集分组任务
        deviceManager->registerAllGroupTasks();
        // // 7. 启动调度器
        timerScheduler->start();
        GLOG_INFO("IoT Gateway Started. Press Ctrl+C to exit.");
        // // 8. 阻塞主线程（可按需用信号优雅退出）
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }catch (const std::exception& e) {
        std::cerr << "捕获异常: " << e.what() << std::endl;
    }
}
