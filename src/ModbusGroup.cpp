#include "ModbusGroup.h"
#include "ModbusDevice.h"
#include "ModbusVariable.h"
#include "DataBuffer.h"
#include "Logger.h"
#include <chrono>
#include <vector>
#include <string>
#include <variant>
#include <iomanip>
#include <sstream>
#include "DeviceManager.h"
void ModbusGroup::pollVariablesImpl(const std::shared_ptr<Device> &dev) {
    auto device = std::dynamic_pointer_cast<ModbusDevice>(dev);
    if (!device) {
        GLOG_ERROR("ModbusGroup[" + getId() + "] 设备[" + getDeviceId() + "] 未找到，跳过采集！");
        return;
    }
    auto* modbusDevice = dynamic_cast<ModbusDevice*>(device.get());
    if (!modbusDevice) {
        GLOG_ERROR("ModbusGroup[" + getId() + "] 设备类型错误！");
        return;
    }

    for (const auto& v : getVariables()) {
        auto mbVar = std::dynamic_pointer_cast<ModbusVariable>(v);
        if (!mbVar) continue;
        const int address = mbVar->addressAsInt();
        const int regCount = mbVar->registerCount();
        ModbusRegisterArea area = guessAreaFromAddress(address);
        const int realAddress=stripAreaPrefix(address);
        bool ok = false;
        std::vector<uint16_t> regs;
        std::vector<uint8_t>  bits;

        switch (area) {
            case ModbusRegisterArea::Coil:
                ok = modbusDevice->readCoils(realAddress, regCount, bits);
                if (ok) mbVar->setRawBits(bits); // 你要实现 setRawBits 支持 bool
                break;
            case ModbusRegisterArea::DiscreteInput:
                ok = modbusDevice->readDiscreteInputs(realAddress, regCount, bits);
                if (ok) mbVar->setRawBits(bits);
                break;
            case ModbusRegisterArea::InputRegister:
                ok = modbusDevice->readInputRegisters(realAddress, regCount, regs);
                if (ok) mbVar->setRawValue(regs);
                break;
            case ModbusRegisterArea::HoldingRegister:
                ok = modbusDevice->readRegisters(realAddress, regCount, regs);
                if (ok) mbVar->setRawValue(regs);
                break;
            default:
                GLOG_ERROR("ModbusGroup[" + getId() + "] 变量[" + mbVar->getVarid() + "] 地址[" + std::to_string(address) + "] 未识别寄存器区，跳过！");
                continue;
        }

        if (ok) {
            mbVar->setQuality(VarQuality::GOOD);
            std::string valueStr;
            const auto& value = mbVar->getValue();
            std::visit([&](auto&& vv){
                using T = std::decay_t<decltype(vv)>;
                if constexpr (std::is_same_v<T, std::string>)
                    valueStr = vv;
                else if constexpr (std::is_same_v<T, bool>)
                    valueStr = vv ? "true" : "false";
                else
                    valueStr = std::to_string(vv);
            }, value);

            auto tp = mbVar->getTimestamp();
            std::time_t t = std::chrono::system_clock::to_time_t(tp);
            std::tm tm{};
            localtime_s(&tm, &t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

            std::string qualityStr;
            switch (mbVar->getQuality()) {
                case VarQuality::GOOD: qualityStr = "GOOD"; break;
                case VarQuality::BAD: qualityStr = "BAD"; break;
                case VarQuality::UNCERTAIN: qualityStr = "UNCERTAIN"; break;
                default: qualityStr = "?"; break;
            }

            std::ostringstream msg;
            msg << "ModbusGroup[" << getId() << "] 变量[" << mbVar->getVarid() << "] = "
                << valueStr << " [Time=" << oss.str() << ", Quality=" << qualityStr << "]";
            GLOG_DEBUG(msg.str());
            DataBuffer::instance().set(
                mbVar->getVarid(),
                mbVar->getValue(),
                mbVar->getTimestamp(),
                mbVar->getQuality()
            );
        } else {
            mbVar->setQuality(VarQuality::BAD);
            DataBuffer::instance().set(
                mbVar->getVarid(),
                mbVar->getValue(),
                std::chrono::system_clock::now(),
                VarQuality::BAD
            );
            GLOG_DEBUG("ModbusGroup[" + getId() + "] 变量[" + mbVar->getVarid() + "] 采集失败，已置BAD");
        }
    }
}
