#include "Variable.h"
#include <stdexcept>
#include <algorithm>
#include <utility>

Variable::Variable(std::string  id,
                   std::string  name,
                   std::wstring  address,
                   const VarType type,
                   const VarAccess access)
    : id_(std::move(id)), name_(std::move(name)), address_(std::move(address)),
      type_(type), access_(access), quality_(VarQuality::UNCERTAIN) {}

void Variable::setValue(const ValueType& value, const VarQuality quality) {
    value_ = value;
    quality_ = quality;
    timestamp_ = std::chrono::system_clock::now();
}

VarAccess Variable::parseAccess(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str == "ro") return VarAccess::RO;
    if (str == "rw") return VarAccess::RW;
    if (str == "wo") return VarAccess::WO;
    throw std::invalid_argument("Unknown VarAccess: " + s);
}
std::string Variable::accessToString(VarAccess access) {
    switch (access) {
        case VarAccess::RO: return "RO";
        case VarAccess::RW: return "RW";
        case VarAccess::WO: return "WO";
        default: return "?";
    }
}

VarType Variable::parseType(const std::string& s) {
    std::string str = s;
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    if (str == "bool")   return VarType::BOOL;
    if (str == "int16")  return VarType::INT16;
    if (str == "uint16") return VarType::UINT16;
    if (str == "int32")  return VarType::INT32;
    if (str == "uint32") return VarType::UINT32;
    if (str == "int64")  return VarType::INT64;
    if (str == "uint64") return VarType::UINT64;
    if (str == "float")  return VarType::FLOAT;
    if (str == "double") return VarType::DOUBLE;
    throw std::invalid_argument("Unknown VarType: " + s);
}
std::string Variable::typeToString(const VarType type) {
    switch(type) {
        case VarType::BOOL:   return "bool";
        case VarType::INT16:  return "int16";
        case VarType::UINT16: return "uint16";
        case VarType::INT32:  return "int32";
        case VarType::UINT32: return "uint32";
        case VarType::INT64:  return "int64";
        case VarType::UINT64: return "uint64";
        case VarType::FLOAT:  return "float";
        case VarType::DOUBLE: return "double";
        default: return "?";
    }
}
