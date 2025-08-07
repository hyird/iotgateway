#include "ModbusVariable.h"
#include <cstring>
#include <algorithm>

int ModbusVariable::addressAsInt() const {
    try {
        if (address_.rfind(L"0x", 0) == 0 || address_.rfind(L"0X", 0) == 0)
            return std::stoi(address_, nullptr, 16);
        else
            return std::stoi(address_);
    } catch (...) {
        return 0;
    }
}


int ModbusVariable::registerCount() const {
    switch (type_) {
        case VarType::BOOL:   return 1;
        case VarType::INT16:  case VarType::UINT16: return 1;
        case VarType::INT32:  case VarType::UINT32: case VarType::FLOAT: return 2;
        case VarType::INT64:  case VarType::UINT64: case VarType::DOUBLE: return 4;
        default: return 1;
    }
}

void ModbusVariable::setRawBits(const std::vector<uint8_t>& bits) {
    if (type_ == VarType::BOOL) {
        if (bits.empty()) {
            return;
        }
        value_ = (bits[0] != 0);
        timestamp_ = std::chrono::system_clock::now();
    } else {
    }
}

void ModbusVariable::setRawValue(const std::vector<uint16_t>& regs) {
    rawRegs_ = regs;
    value_ = decodeValue();
    timestamp_ = std::chrono::system_clock::now();
}

ModbusVariable::ValueType ModbusVariable::decodeValue() const {
    return decodeValue(endianness_, byteSwap_);
}

ModbusVariable::ValueType ModbusVariable::decodeValue(const std::string& endianness, bool byteSwap) const {
    if (rawRegs_.empty()) return {};

    // 16位类型直接返回
    if (type_ == VarType::BOOL)
        return (rawRegs_[0] != 0);
    if (type_ == VarType::INT16)
        return static_cast<int16_t>(rawRegs_[0]);
    if (type_ == VarType::UINT16)
        return rawRegs_[0];

    // 32位 float/int32/uint32
    if ((type_ == VarType::FLOAT || type_ == VarType::INT32 || type_ == VarType::UINT32) && rawRegs_.size() >= 2) {
        uint16_t r0 = rawRegs_[0], r1 = rawRegs_[1];
        // abcd:    high=reg[0], low=reg[1]
        // cdab:    high=reg[1], low=reg[0]
        // badc:    high=qToBigEndian(reg[0]), low=qToBigEndian(reg[1])
        // dcba:    high=qToBigEndian(reg[1]), low=qToBigEndian(reg[0])
        float f;
        uint32_t u32;
        int32_t i32;
        if (endianness == "big" && !byteSwap) {      // abcd
            u32 = (uint32_t(r0) << 16) | r1;
        } else if (endianness == "little" && !byteSwap) { // dcba
            u32 = (uint32_t(toBigEndian(r1)) << 16) | toBigEndian(r0);
        } else if (endianness == "big" && byteSwap) {    // badc
            u32 = (uint32_t(toBigEndian(r0)) << 16) | toBigEndian(r1);
        } else if (endianness == "little" && byteSwap) { // cdab
            u32 = (uint32_t(r1) << 16) | r0;
        } else { // fallback abcd
            u32 = (uint32_t(r0) << 16) | r1;
        }

        if (type_ == VarType::FLOAT) {
            std::memcpy(&f, &u32, 4);
            return f;
        } else if (type_ == VarType::INT32) {
            std::memcpy(&i32, &u32, 4);
            return i32;
        } else {
            return u32;
        }
    }

    // 64位 double/int64/uint64
    if ((type_ == VarType::DOUBLE || type_ == VarType::INT64 || type_ == VarType::UINT64) && rawRegs_.size() >= 4) {
        uint16_t r[4] = {rawRegs_[0], rawRegs_[1], rawRegs_[2], rawRegs_[3]};
        uint64_t u64 = 0;
        if (endianness == "big" && !byteSwap) { // abcdefgh
            u64 = (uint64_t(r[0]) << 48) | (uint64_t(r[1]) << 32) | (uint64_t(r[2]) << 16) | r[3];
        } else if (endianness == "little" && !byteSwap) { // hgfedcba
            u64 = (uint64_t(toBigEndian(r[3])) << 48) | (uint64_t(toBigEndian(r[2])) << 32) |
                  (uint64_t(toBigEndian(r[1])) << 16) | toBigEndian(r[0]);
        } else if (endianness == "big" && byteSwap) { // badcfehg
            u64 = (uint64_t(toBigEndian(r[0])) << 48) | (uint64_t(toBigEndian(r[1])) << 32) |
                  (uint64_t(toBigEndian(r[2])) << 16) | toBigEndian(r[3]);
        } else if (endianness == "little" && byteSwap) { // ghefcdab
            u64 = (uint64_t(r[3]) << 48) | (uint64_t(r[2]) << 32) | (uint64_t(r[1]) << 16) | r[0];
        } else { // fallback
            u64 = (uint64_t(r[0]) << 48) | (uint64_t(r[1]) << 32) | (uint64_t(r[2]) << 16) | r[3];
        }

        if (type_ == VarType::DOUBLE) {
            double d;
            std::memcpy(&d, &u64, 8);
            return d;
        } else if (type_ == VarType::INT64) {
            int64_t i64;
            std::memcpy(&i64, &u64, 8);
            return i64;
        } else {
            return u64;
        }
    }

    return {};
}