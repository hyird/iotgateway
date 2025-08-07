#include "Group.h"
#include <utility>
#include "DeviceManager.h"
#include "Logger.h"

Group::Group(DeviceManager* mgr, std::string  deviceId, std::string  id,
             std::string  name, const uint32_t intervalMs)
    : mgr_(mgr), deviceId_(std::move(deviceId)), id_(std::move(id)), name_(std::move(name)), intervalMs_(intervalMs), active_(true) {}

void Group::addVariable(const std::shared_ptr<Variable>& var) {
    variables_.push_back(var);
}

std::vector<std::shared_ptr<Variable>>& Group::getVariables() { return variables_; }
const std::vector<std::shared_ptr<Variable>>& Group::getVariables() const { return variables_; }
uint32_t Group::getIntervalMs() const { return intervalMs_; }
std::string Group::getId() const { return id_; }
std::string Group::getDeviceId() const { return deviceId_; }

void Group::pollVariables() {
    const auto dev = mgr_->getDevice(getDeviceId());
    if (!dev) {
        GLOG_ERROR("Group[" + getId() + "] 无法获取Device实例");
        return;
    }
    if (!dev->tryEnterPoll()) {
        return;
    }
    try {
        pollVariablesImpl(dev);
    } catch (const std::exception &ex) {
        GLOG_ERROR("Group[" + getId() + "] pollVariablesImpl异常: " + std::string(ex.what()));
        dev->reportError("OpcdaGroup[" + getId() + "] pollVariablesImpl异常: " + ex.what());
    } catch (...) {
        GLOG_ERROR("Group[" + getId() + "] pollVariablesImpl未知异常");
        dev->reportError("Group[" + getId() + "] pollVariablesImpl未知异常");
    }
    dev->exitPoll();
}