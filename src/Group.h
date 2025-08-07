#pragma once
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include "Variable.h"

class Device;
// 前置声明
class DeviceManager;

class Group {
public:
    Group(DeviceManager* mgr, std::string  deviceId, std::string  id,
          std::string  name, uint32_t intervalMs);
    virtual ~Group() = default;

    virtual void pollVariablesImpl(const std::shared_ptr<Device>& device) = 0;
    virtual void pollVariables();
    void addVariable(const std::shared_ptr<Variable>& var);
    std::vector<std::shared_ptr<Variable>>& getVariables();
    [[nodiscard]] const std::vector<std::shared_ptr<Variable>>& getVariables() const;
    uint32_t getIntervalMs() const;
    std::string getId() const;
    std::string getDeviceId() const;
    DeviceManager* getDeviceManager() const { return mgr_; }
    friend Device;
protected:
    DeviceManager* mgr_;
    std::string deviceId_;
    std::string id_;
    std::string name_;
    const uint32_t intervalMs_;
    std::vector<std::shared_ptr<Variable>> variables_;
    std::atomic<bool> active_;
};
