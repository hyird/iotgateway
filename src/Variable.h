#pragma once
#include <string>
#include <variant>
#include <chrono>

enum class VarType { BOOL, INT16, UINT16, INT32, UINT32, INT64, UINT64, FLOAT, DOUBLE };
enum class VarQuality { GOOD, BAD, UNCERTAIN };
enum class VarAccess { RO, RW, WO };

class Variable {
public:
    using ValueType = std::variant<std::string,bool, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double>;

    Variable(std::string  id,
             std::string  name,
             std::wstring  address,
             VarType type,
             VarAccess access);
    virtual ~Variable() = default;
    void setValue(const ValueType& value, VarQuality quality);
    [[nodiscard]] ValueType getValue() const { return value_; }
    void setQuality(const VarQuality quality) { quality_ = quality; }
    [[nodiscard]] VarQuality getQuality() const { return quality_; }
    [[nodiscard]] std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }
    [[nodiscard]] std::string getId() const { return id_; }
    [[nodiscard]] std::string getName() const { return name_; }
    [[nodiscard]] std::wstring getAddress() const { return address_; }
    [[nodiscard]] VarType getType() const { return type_; }
    [[nodiscard]] VarAccess getAccess() const { return access_; }
    [[nodiscard]] std::string getVarid() const { return id_; }
    static VarType parseType(const std::string& s);
    static VarAccess parseAccess(const std::string& s);
    static std::string typeToString(VarType type);
    static std::string accessToString(VarAccess access);

protected:
    std::string id_;
    std::string name_;
    std::wstring address_;
    VarType type_;
    VarAccess access_;
    ValueType value_;
    VarQuality quality_;
    std::chrono::system_clock::time_point timestamp_;
};
