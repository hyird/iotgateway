#include "ModbusDevice.h"
#include <thread>
#include <cerrno>
#include <utility>
#include "Logger.h"

ModbusDevice::ModbusDevice(const std::string& id,
                           const std::string& name,
                           std::string  ip,
                           const int port,
                           const int slaveId,
                           std::string  endianness,
                           const bool  byteSwap,
                           const int failThreshold,
                           const int reconnectIntervalMs)
    : Device(id, name),
      ctx_(nullptr),
      ip_(std::move(ip)),
      port_(port),
      slaveId_(slaveId),
      endianness_(std::move(endianness)),
      byteSwap_(byteSwap),
      failCount_(0),
      reconnecting_(false),
      online_(false),
      failThreshold_(failThreshold),
      reconnectIntervalMs_(reconnectIntervalMs)
{}

ModbusDevice::~ModbusDevice() {
    disconnect();
}

bool ModbusDevice::connect() {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
    }
    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    if (!ctx_) {
        lastError_ = "创建modbus句柄失败";
        GLOG_ERROR(logPrefix() + lastError_);
        return false;
    }
    if (modbus_connect(ctx_) == -1) {
        lastError_ = std::string("modbus_connect 失败: ") + modbus_strerror(errno);
        GLOG_ERROR(logPrefix() + lastError_);
        modbus_free(ctx_);
        ctx_ = nullptr;
        return false;
    }
    modbus_set_slave(ctx_, slaveId_);
    online_ = true;
    GLOG_INFO(logPrefix() + "连接成功");
    return true;
}

void ModbusDevice::disconnect() {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
        GLOG_INFO(logPrefix() + "已断开连接");
    }
    online_ = false;
}

bool ModbusDevice::readRegisters(const int addr, const int count, std::vector<uint16_t>& regs) {
    std::unique_lock<std::mutex> lock(comm_mtx_);
    const auto now = std::chrono::steady_clock::now();

    if (!online_ && now - lastFailTime_ < std::chrono::milliseconds(reconnectIntervalMs_)) {
        lastError_ = "设备已断线，延迟重连，跳过本次采集";
        return false;
    }
    if (!ctx_ || !online_) {
        if (reconnecting_) {
            lastError_ = "设备正在重连，跳过本次采集";
            GLOG_WARN(logPrefix() + lastError_);
            return false;
        }
        reconnecting_ = true;
        lock.unlock();
        const bool connOk = connect();
        lock.lock();
        reconnecting_ = false;
        if (!connOk) {
            online_ = false;
            lastFailTime_ = now;
            lastError_ = "连接失败！";
            GLOG_ERROR(logPrefix() + lastError_);
            return false;
        }
        online_ = true;
        failCount_ = 0;
    }
    regs.resize(count);
    if (const int rc = modbus_read_registers(ctx_, addr, count, regs.data()); rc == count) {
        if (!online_) {
            GLOG_INFO(logPrefix() + "通信恢复正常！");
            online_ = true;
            failCount_ = 0;
        }
        lastError_.clear();
        return true;
    }

    failCount_++;
    lastError_ = "采集失败，failCount=" + std::to_string(failCount_);
    GLOG_WARN(logPrefix() + lastError_);
    if (failCount_ >= failThreshold_) {
        GLOG_ERROR(logPrefix() + "连续多次失败，设备判定为掉线");
        online_ = false;
        lastFailTime_ = now;
        failCount_ = 0;
        disconnect();
    } else {
        disconnect();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (const bool connOk = connect(); !connOk) {
            GLOG_WARN(logPrefix() + "快速重连失败");
            online_ = false;
            lastFailTime_ = now;
        }
    }
    return false;
}

bool ModbusDevice::readInputRegisters(const int addr, const int count, std::vector<uint16_t>& regs) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    regs.resize(count);
    if (const int rc = modbus_read_input_registers(ctx_, addr, count, regs.data()); rc == count) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "readInputRegisters 失败";
        return false;
    }
}

bool ModbusDevice::readCoils(const int addr, const int count, std::vector<uint8_t>& coils) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    coils.resize(count);
    if (const int rc = modbus_read_bits(ctx_, addr, count, coils.data()); rc == count) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "readCoils 失败";
        return false;
    }
}

bool ModbusDevice::readDiscreteInputs(const int addr, const int count, std::vector<uint8_t>& inputs) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    inputs.resize(count);
    if (const int rc = modbus_read_input_bits(ctx_, addr, count, inputs.data()); rc == count) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "readDiscreteInputs 失败";
        return false;
    }
}

bool ModbusDevice::writeSingleRegister(const int addr, const uint16_t value) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    int rc = modbus_write_register(ctx_, addr, value);
    if (rc == 1) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "writeSingleRegister 失败";
        return false;
    }
}

bool ModbusDevice::writeMultipleRegisters(const int addr, const std::vector<uint16_t>& values) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    if (const int rc = modbus_write_registers(ctx_, addr, static_cast<int>(values.size()), values.data()); rc == values.size()) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "writeMultipleRegisters 失败";
        return false;
    }
}

bool ModbusDevice::writeSingleCoil(const int addr, const bool on) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    if (const int rc = modbus_write_bit(ctx_, addr, on ? 1 : 0); rc == 1) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "writeSingleCoil 失败";
        return false;
    }
}

bool ModbusDevice::writeMultipleCoils(const int addr, const std::vector<uint8_t>& values) {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    if (const int rc = modbus_write_bits(ctx_, addr, static_cast<int>(values.size()), values.data()); rc == values.size()) {
        lastError_.clear();
        return true;
    } else {
        lastError_ = "writeMultipleCoils 失败";
        return false;
    }
}

bool ModbusDevice::isConnected() const {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    return ctx_ != nullptr && online_;
}

bool ModbusDevice::isOnline() const {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    return online_;
}

std::string ModbusDevice::getStatusString() const {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    if (online_)
        return "ONLINE";
    else
        return "OFFLINE(" + lastError_ + ")";
}

std::string ModbusDevice::getLastError() const {
    std::lock_guard<std::mutex> lock(comm_mtx_);
    return lastError_;
}

std::string ModbusDevice::logPrefix() const {
    return "Modbus[" + name_ + "] ";
}
