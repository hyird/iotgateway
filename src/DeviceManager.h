#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Device.h"
#include "ThreadPool.h"
#include "TimerScheduler.h"
#include "JsonConfig.h"

class Group;

class DeviceManager : public std::enable_shared_from_this<DeviceManager> {
public:
    static std::shared_ptr<DeviceManager> create(
        std::shared_ptr<ThreadPool> pool,
        std::shared_ptr<TimerScheduler> scheduler,
        const GlobalConfig& cfg);

    void registerAllGroupTasks();
    void unregisterGroupTasksByDevice(const std::string& devId);
    void reRegisterDeviceTasks(const std::string& devId);
    std::shared_ptr<Device> getDevice(const std::string& id);

private:
    DeviceManager(std::shared_ptr<ThreadPool> pool,
                  std::shared_ptr<TimerScheduler> scheduler);
    void initDevices(const GlobalConfig& cfg);
    std::unordered_map<std::string, std::shared_ptr<Device>> devices_;
    std::shared_ptr<ThreadPool> pool_;
    std::shared_ptr<TimerScheduler> scheduler_;
    std::unordered_map<std::string, std::unordered_map<std::string, TimerScheduler::TimerHandle>> groupTaskHandles_;
};
