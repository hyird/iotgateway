#pragma once
#include <stdexcept>
#include <string>
#include <vector>

// 变量配置
struct VariableConfig {
    std::string id;
    std::string name;
    std::string type;
    std::wstring address;
    int length = 0;
    bool persist_on_change = false;
    std::string access = "RO";
    static int regLengthFromType(const std::string& type) {
        if (type == "bool" || type == "int16" || type == "uint16") return 1;
        if (type == "int32" || type == "uint32" || type == "float") return 2;
        if (type == "int64" || type == "uint64" || type == "double") return 4;
        throw std::runtime_error("Unknown Modbus variable type: " + type);
    }
};

struct GroupConfig {
    std::string id;
    std::string name;
    int interval_ms = 1000;
    bool persist_on_change = false;
    std::vector<VariableConfig> variables;
};

// 通用设备配置，所有协议的字段都放进来，不用的就是默认值
struct DeviceConfig {
    std::string id;
    std::string name;
    std::string type;

    // modbus 专用
    std::string protocol;    // modbus
    std::string ip;          // modbus
    int port = 0;            // modbus
    int slave_id = 0;        // modbus
    std::string endianness;  // modbus
    bool byte_swap = false;  // modbus

    // opcda 专用
    std::string host;        // opcda
    std::string servername;  // opcda

    std::vector<GroupConfig> groups;
};

struct StorageConfig {
    std::string type;
    std::string host;
    int port = 0;
    std::string user;
    std::string password;
    std::string database;
    std::string table;
    bool write_on_change = true;
    int write_on_interval_ms = 0;
    std::vector<std::string> fields;
};

struct SystemConfig {
    int thread_pool_size = 32;
    std::string log_level = "info";
    std::string log_file = "logs/gateway.log";
};

struct GlobalConfig {
    SystemConfig system;
    StorageConfig storage;
    std::vector<DeviceConfig> devices;
};

class JsonConfig {
public:
    static GlobalConfig load(const std::string& path);
};
