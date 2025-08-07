// OpcdaDevice.h
#pragma once
#include <string>
#include <mutex>
#include <unordered_map>
#include "Device.h"
#include "OPCServer.h"

class COPCClient;
class COPCHost;
class COPCServer;

class OpcdaDevice final : public Device {
public:
    OpcdaDevice(const std::string& id,
                const std::string& name,
                std::string  host,
                std::string  serverName);
    bool connect() override;
    void disconnect() override;
    void beforeDisconnect() override;
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] std::string logPrefix() const;
    [[nodiscard]] COPCServer* getServer() const;
    ServerStatus getStatus() const;
    static std::wstring utf8_to_wstring(const std::string& str);
    static std::string wstring_to_utf8(const std::wstring& wstr);
    std::shared_ptr<COPCGroup> getOrCreateGroup(
        const std::string &groupId,
        bool active,
        unsigned long reqUpdateRateMs,
        float deadBand,
        bool enableAsync,
        IAsyncDataCallback *callback
    );
    bool enableAsync(const std::string& groupId, IAsyncDataCallback* cb);
    void removeGroup(const std::string& groupId);


private:
    std::string host_;
    std::string serverName_;
    COPCHost* opcHOST_{};
    COPCServer* opcServer_{};
    ServerStatus status_{};
    bool connected_ = false;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, std::shared_ptr<COPCGroup>> groups_;
};

