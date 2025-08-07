#include "DeviceManager.h"
#include "ModbusDevice.h"
#include "ModbusGroup.h"
#include "ModbusVariable.h"
#include "OpcdaDevice.h"
#include "OpcdaGroup.h"
#include "OpcdaVariable.h"
#include "Logger.h"
#include <utility>
#include <iostream>

// 工厂函数，负责托管shared_ptr并完成setManager
std::shared_ptr<DeviceManager> DeviceManager::create(
    std::shared_ptr<ThreadPool> pool,
    std::shared_ptr<TimerScheduler> scheduler,
    const GlobalConfig& cfg)
{
    // 注意此处必须new出来，不能直接make_shared(DeviceManager(...))，因为private构造
    auto mgr = std::shared_ptr<DeviceManager>(new DeviceManager(pool, scheduler));
    mgr->initDevices(cfg);
    return mgr;
}

// 私有构造
DeviceManager::DeviceManager(std::shared_ptr<ThreadPool> pool,
                             std::shared_ptr<TimerScheduler> scheduler)
    : pool_(std::move(pool)), scheduler_(std::move(scheduler))
{}

// 真正初始化设备
void DeviceManager::initDevices(const GlobalConfig& cfg)
{
    for (const auto& devConf : cfg.devices) {
        std::shared_ptr<Device> dev;
        if (devConf.type == "modbus") {
            dev = std::make_shared<ModbusDevice>(
                devConf.id, devConf.name,
                devConf.ip, devConf.port, devConf.slave_id,
                devConf.endianness, devConf.byte_swap
            );
        } else if (devConf.type == "opcda") {
            dev = std::make_shared<OpcdaDevice>(
                devConf.id, devConf.name, devConf.host, devConf.servername
            );
        } else {
            GLOG_WARN("暂不支持的设备类型: " + devConf.type);
            continue;
        }
        // group/variable
        for (const auto& grpConf : devConf.groups) {
            std::shared_ptr<Group> grp;
            if (devConf.type == "modbus") {
                grp = std::make_shared<ModbusGroup>(
                    this, devConf.id, grpConf.id, grpConf.name, grpConf.interval_ms
                );
            } else if (devConf.type == "opcda") {
                grp = std::make_shared<OpcdaGroup>(
                    this, devConf.id, grpConf.id, grpConf.name, grpConf.interval_ms
                );
            }
            for (const auto& varConf : grpConf.variables) {
                if (devConf.type == "modbus") {
                    VarType type = Variable::parseType(varConf.type);
                    VarAccess access = Variable::parseAccess(varConf.access);
                    auto var = std::make_shared<ModbusVariable>(
                        varConf.id, varConf.name, varConf.address, type, access,
                        devConf.endianness, devConf.byte_swap
                    );
                    grp->addVariable(var);
                } else if (devConf.type == "opcda") {
                    VarType type = Variable::parseType(varConf.type);
                    VarAccess access = Variable::parseAccess(varConf.access);
                    auto var = std::make_shared<OpcdaVariable>(
                        varConf.id, varConf.name, varConf.address, type, access
                    );
                    grp->addVariable(var);
                }
            }
            dev->getGroups().push_back(grp);
        }
        devices_[devConf.id] = dev;
    }
    // *** 初始化完所有device后，再setManager ***
    for (auto& [id, dev] : devices_) {
        dev->setManager(shared_from_this());
    }
}

std::shared_ptr<Device> DeviceManager::getDevice(const std::string& id) {
    const auto it = devices_.find(id);
    return it != devices_.end() ? it->second : nullptr;
}

void DeviceManager::registerAllGroupTasks() {
    for (const auto& [devId, dev] : devices_) {
        for (const auto& grp : dev->getGroups()) {
            auto pollFunc = [grp]() { grp->pollVariables(); };
            const auto handle = scheduler_->scheduleEvery(grp->getIntervalMs(), pollFunc);
            groupTaskHandles_[devId][grp->getId()] = handle;
            GLOG_INFO("注册分组定时任务: 设备=" + devId + " 分组=" + grp->getId());
        }
    }
}

void DeviceManager::unregisterGroupTasksByDevice(const std::string& devId) {
    if (const auto it = groupTaskHandles_.find(devId); it != groupTaskHandles_.end()) {
        for (const auto& [grpId, handle] : it->second) {
            scheduler_->cancel(handle);
            GLOG_INFO("注销分组定时任务: 设备=" + devId + " 分组=" + grpId);
        }
        groupTaskHandles_.erase(it);
    }
}

void DeviceManager::reRegisterDeviceTasks(const std::string& devId) {
    unregisterGroupTasksByDevice(devId);
    if (const auto it = devices_.find(devId); it != devices_.end()) {
        const auto& dev = it->second;
        for (const auto& grp : dev->getGroups()) {
            auto pollFunc = [grp]() { grp->pollVariables(); };
            const auto handle = scheduler_->scheduleEvery(grp->getIntervalMs(), pollFunc);
            groupTaskHandles_[devId][grp->getId()] = handle;
            GLOG_INFO("重新注册分组定时任务: 设备=" + devId + " 分组=" + grp->getId());
        }
    }
}
