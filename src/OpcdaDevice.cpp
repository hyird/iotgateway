// OpcdaDevice.cpp

#include "OpcdaDevice.h"
#include <utility>
#include <windows.h>
#include "Logger.h"
#include "OpcdaGroup.h"
#include "OPCHost.h"

OpcdaDevice::OpcdaDevice(const std::string& id,
                         const std::string& name,
                         std::string  host,
                         std::string  serverName)
    : Device(id, name), host_(std::move(host)), serverName_(std::move(serverName))
{
    COPCClient::init(MULTITHREADED);
}

bool OpcdaDevice::connect() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (connected_) {
        GLOG_INFO(logPrefix() + "无需再连");
        return true;
    }
    try{
        constexpr char c_string[100] = "localhost";
        if(opcHOST_==nullptr){
            opcHOST_ = COPCClient::makeHost(COPCHost::S2WS(c_string));
        }
        if(opcServer_==nullptr){
            opcServer_ = opcHOST_->connectDAServer(utf8_to_wstring(serverName_));
        }
        do{
            opcServer_->getStatus(status_);
        } while (status_.dwServerState != OPC_STATUS_RUNNING);
        GLOG_INFO(logPrefix() + "连接成功");
        connected_= true;
    }
    catch(const OPCException& ex){
        connected_=false;
    }
    catch(...){
        connected_=false;
    }
    return connected_;
}

void OpcdaDevice::disconnect() {
    beforeDisconnect();
    std::lock_guard<std::mutex> lock(mtx_);
    connected_ = false;
    if (opcServer_) {
        delete opcServer_;
        opcServer_ = nullptr;
    }
    if (opcHOST_) {
        delete opcHOST_;
        opcHOST_ = nullptr;
    }
    GLOG_INFO(logPrefix() + "已断开连接");
}
void OpcdaDevice::beforeDisconnect() {
    // for (auto& grp : groups_) {
    //     if (const auto opcdaGrp = std::dynamic_pointer_cast<OpcdaGroup>(grp) ) {
    //         if (opcdaGrp->group_) {
    //             if (opcdaGrp->isEnabledAsync) {
    //                 opcdaGrp->group_->disableAsync();
    //             }
    //             opcdaGrp->group_.reset();
    //         }
    //     }
    // }
}


bool OpcdaDevice::isConnected() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return connected_;
}

std::string OpcdaDevice::logPrefix() const {
    return "Opcda[" + name_ + "] ";
}

COPCServer* OpcdaDevice::getServer() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return opcServer_;
}

ServerStatus OpcdaDevice::getStatus() const {
    return status_;
}

std::wstring OpcdaDevice::utf8_to_wstring(const std::string& str) {
    if (str.empty()) return {};
    const int sz = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring wstr(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &wstr[0], sz);
    return wstr;
}

std::string OpcdaDevice::wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    const int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string res(len, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &res[0], len, nullptr, nullptr);
    return res;
}

std::shared_ptr<COPCGroup> OpcdaDevice::getOrCreateGroup(
    const std::string &groupId,
    const bool active,
    const unsigned long reqUpdateRateMs,
    const float deadBand,
    const bool enableAsync,
    IAsyncDataCallback *callback
    )
{
    if (const auto it = groups_.find(groupId); it != groups_.end() && it->second) {
        return it->second;
    }
    if (!isConnected()) {
        if (!connect()) {
            GLOG_ERROR("OpcdaGroup[" + getId() + "] 连接OPC DA失败！");
        }
    }
    std::lock_guard<std::mutex> lock(mtx_);
    if (!opcServer_) {
        GLOG_ERROR("OpcdaDevice::getOrCreateGroup: opcServer_ is nullptr!");
        return nullptr;
    }
    unsigned long revisedUpdateRate = 0;
    COPCGroup* rawGroup = opcServer_->makeGroup(
        utf8_to_wstring(groupId),
        active,
        reqUpdateRateMs,
        revisedUpdateRate,
        deadBand
    );

    if (!rawGroup) {
        GLOG_ERROR("OpcdaDevice::getOrCreateGroup: 创建Group失败!");
        return nullptr;
    }

    if (enableAsync && callback) {
        rawGroup->enableAsync(callback);
    }

    std::shared_ptr<COPCGroup> groupPtr(rawGroup, [](const COPCGroup* g) {
        delete g;
    });
    groups_[groupId] = groupPtr;
    GLOG_INFO("OpcdaDevice: 创建OPC组 [" + groupId + "] 成功");
    return groupPtr;
}

void OpcdaDevice::removeGroup(const std::string& groupId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    groups_.erase(groupId);
    GLOG_INFO("OpcdaDevice: OPC组 [" + groupId + "] 已移除");
}
