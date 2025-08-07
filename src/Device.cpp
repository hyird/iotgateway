#include "Device.h"

#include "DeviceManager.h"
#include "Logger.h"

Device::Device(const std::string& id, const std::string& name)
    : id_(id), name_(name) {}

Device::~Device() {}

std::string Device::getId() const { return id_; }
std::string Device::getName() const { return name_; }
std::vector<std::shared_ptr<Group>>& Device::getGroups() { return groups_; }

void Device::setManager(const std::shared_ptr<DeviceManager>& mgr) {
    mgr_ = mgr;
}

void Device::setErrorRetryThreshold(int n) {
    errorRetryThreshold_ = n > 0 ? n : 3;
}

bool Device::tryEnterPoll() {
    if (inError_) return false;
    ++runningPollCount_;
    return true;
}
void Device::exitPoll() {
    --runningPollCount_;
}
void Device::resetErrorCount() {
    errorCount_ = 0;
}

void Device::reportError(const std::string& errMsg) {
    if (const int ec = ++errorCount_; ec < errorRetryThreshold_) {
        std::lock_guard<std::mutex> lk(errMtx_);
        lastErrorMsg_ = errMsg;
        return;
    }
    if (bool expected = false; inError_.compare_exchange_strong(expected, true)) {
        {
            std::lock_guard<std::mutex> lk(errMtx_);
            lastErrorMsg_ = errMsg;
        }
        if (const auto mgr = mgr_.lock()) {
            mgr->unregisterGroupTasksByDevice(id_);
        }
        std::weak_ptr self = shared_from_this();
        std::thread([self, this](){
            if (const auto s = self.lock()) {
                while (s->runningPollCount_ > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                s->disconnect();
                s->inError_ = false;
                std::this_thread::sleep_for(std::chrono::seconds(15));
                s->errorCount_ = 0;
                if (const auto mgr = s->mgr_.lock()) {
                    mgr->reRegisterDeviceTasks(s->id_);
                }
            }
        }).detach();
    }
}