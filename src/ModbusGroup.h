#pragma once
#include "Group.h"

class ModbusGroup final : public Group {
public:
    using Group::Group;
    enum class ModbusRegisterArea {
        Coil, DiscreteInput, InputRegister, HoldingRegister, Unknown
    };
    static ModbusRegisterArea guessAreaFromAddress(const int address) {
        if (address >= 0 && address <= 9999)
            return ModbusRegisterArea::Coil;
        if (address >= 10000 && address <= 19999)
            return ModbusRegisterArea::DiscreteInput;
        if (address >= 30000 && address <= 39999)
            return ModbusRegisterArea::InputRegister;
        if (address >= 40000 && address <= 49999)
            return ModbusRegisterArea::HoldingRegister;
        return ModbusRegisterArea::Unknown;
    }
    static int stripAreaPrefix(const int address) {
        return address % 10000;
    }
    void pollVariablesImpl(const std::shared_ptr<Device> &dev) override ;
};
