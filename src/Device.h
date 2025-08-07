#pragma once
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "Group.h"

class DeviceManager;

class Device : public std::enable_shared_from_this<Device> {
public:
    Device(const std::string& id, const std::string& name);
    virtual ~Device();
    virtual bool connect() { return true; }
    virtual void disconnect() {beforeDisconnect();}
    virtual void beforeDisconnect() {}
    bool tryEnterPoll();
    void exitPoll();
    void resetErrorCount();
    void reportError(const std::string& errMsg);
    void setErrorRetryThreshold(int n);
    std::string getId() const;
    std::string getName() const;
    std::vector<std::shared_ptr<Group>>& getGroups();
    void setManager(const std::shared_ptr<DeviceManager>& mgr);

protected:
    std::string id_;
    std::string name_;
    std::vector<std::shared_ptr<Group>> groups_;

    std::atomic<int> runningPollCount_{0};
    std::atomic<int> errorCount_{0};
    int errorRetryThreshold_{3};

    std::atomic<bool> inError_{false};
    std::string lastErrorMsg_;
    std::mutex errMtx_;

    std::weak_ptr<DeviceManager> mgr_;
};
