#pragma once
#include "Variable.h"
#include <utility>
#include <vector>

class ModbusVariable final : public Variable {
public:
    ModbusVariable(std::string id, std::string name, std::wstring address,
               const VarType type, const VarAccess access,
               std::string  endianness, const bool byteSwap)
: Variable(std::move(id), std::move(name), std::move(address), type, access),
  endianness_(std::move(endianness)), byteSwap_(byteSwap) {}
    [[nodiscard]] int addressAsInt() const;
    [[nodiscard]] int registerCount() const;
    void setRawBits(const std::vector<uint8_t>& bits);
    void setRawValue(const std::vector<uint16_t>& regs);
    [[nodiscard]] ValueType decodeValue() const;
    [[nodiscard]] ValueType decodeValue(const std::string& endianness, bool byteSwap) const;
    static uint16_t toBigEndian(const uint16_t val) {
        return (val >> 8) | (val << 8);
    }
private:
    std::string endianness_;
    bool byteSwap_;
    std::vector<uint16_t> rawRegs_;
};
