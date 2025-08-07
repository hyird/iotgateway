#pragma once
#include <modbus/modbus.h>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include "Device.h"


class ModbusDevice final : public Device {
public:
    ModbusDevice(const std::string& id,
                 const std::string& name,
                 std::string  ip,
                 int port,
                 int slaveId,
                 std::string  endianness,
                 bool  byteSwap,
                 int failThreshold = 3,
                 int reconnectIntervalMs = 3000);

    ~ModbusDevice() override;

    bool connect() override;
    void disconnect() override;

    // 采集相关接口
    bool readRegisters(int addr, int count, std::vector<uint16_t>& regs);
    bool readInputRegisters(int addr, int count, std::vector<uint16_t>& regs);
    bool readCoils(int addr, int count, std::vector<uint8_t>& coils);
    bool readDiscreteInputs(int addr, int count, std::vector<uint8_t>& inputs);

    bool writeSingleRegister(int addr, uint16_t value);
    bool writeMultipleRegisters(int addr, const std::vector<uint16_t>& values);
    bool writeSingleCoil(int addr, bool on);
    bool writeMultipleCoils(int addr, const std::vector<uint8_t>& values);

    bool isConnected() const;
    bool isOnline()    const;
    std::string getStatusString() const;

    std::string getEndianness() const { return endianness_; }
    bool getByteSwap() const { return byteSwap_;  }

    std::string getLastError() const;

    void setReconnectInterval(const int ms) { reconnectIntervalMs_ = ms; }
    void setFailThreshold(const int n)      { failThreshold_      = n;  }

private:
    std::string logPrefix() const;

    modbus_t* ctx_;
    mutable std::mutex comm_mtx_;
    std::string ip_;
    int port_;
    int slaveId_;
    std::string endianness_;
    bool byteSwap_;

    int failCount_;
    std::chrono::steady_clock::time_point lastFailTime_;
    bool reconnecting_;
    std::atomic<bool> online_;
    int failThreshold_;
    int reconnectIntervalMs_;
    std::string lastError_;
};
