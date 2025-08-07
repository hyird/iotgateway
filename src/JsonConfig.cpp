#include "JsonConfig.h"

#include <codecvt>
#include <boost/json.hpp>
#include <fstream>
#include <iostream>

using namespace boost::json;

inline std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

static VariableConfig parseVariable(const object& o) {
    VariableConfig v;
    v.id = o.at("id").as_string().c_str();
    v.name = o.at("name").as_string().c_str();
    v.type = o.at("type").as_string().c_str();
    v.address = utf8_to_wstring(o.at("address").as_string().c_str());
    v.length = VariableConfig::regLengthFromType(v.type);
    if (o.if_contains("persist_on_change"))
        v.persist_on_change = o.at("persist_on_change").as_bool();
    if (o.if_contains("access"))
        v.access = o.at("access").as_string().c_str();
    return v;
}

static GroupConfig parseGroup(const object& o) {
    GroupConfig g;
    g.id = o.at("id").as_string().c_str();
    g.name = o.at("name").as_string().c_str();
    g.interval_ms = static_cast<int>(o.at("interval_ms").as_int64());
    if (o.if_contains("persist_on_change"))
        g.persist_on_change = o.at("persist_on_change").as_bool();
    for (auto&& vj : o.at("variables").as_array())
        g.variables.push_back(parseVariable(vj.as_object()));
    return g;
}

static DeviceConfig parseDevice(const object& o) {
    DeviceConfig d;
    d.id = o.at("id").as_string().c_str();
    d.name = o.at("name").as_string().c_str();
    d.type = o.at("type").as_string().c_str();

    if (d.type == "modbus") {
        if (o.if_contains("protocol"))   d.protocol = o.at("protocol").as_string().c_str();
        if (o.if_contains("ip"))         d.ip = o.at("ip").as_string().c_str();
        if (o.if_contains("port"))       d.port = static_cast<int>(o.at("port").as_int64());
        if (o.if_contains("slave_id"))   d.slave_id = static_cast<int>(o.at("slave_id").as_int64());
        if (o.if_contains("endianness")) d.endianness = o.at("endianness").as_string().c_str();
        if (o.if_contains("byte_swap"))  d.byte_swap = o.at("byte_swap").as_bool();
    } else if (d.type == "opcda") {
        if (o.if_contains("host"))       d.host = o.at("host").as_string().c_str();
        if (o.if_contains("servername")) d.servername = o.at("servername").as_string().c_str();
    }
    for (auto&& gj : o.at("groups").as_array())
        d.groups.push_back(parseGroup(gj.as_object()));
    return d;
}

static StorageConfig parseStorage(const object& o) {
    StorageConfig s;
    s.type = o.at("type").as_string().c_str();
    s.host = o.at("host").as_string().c_str();
    if (o.if_contains("port"))
        s.port = static_cast<int>(o.at("port").as_int64());
    s.user = o.at("user").as_string().c_str();
    s.password = o.at("password").as_string().c_str();
    s.database = o.at("database").as_string().c_str();
    s.table = o.at("table").as_string().c_str();
    if (o.if_contains("write_on_change"))
        s.write_on_change = o.at("write_on_change").as_bool();
    if (o.if_contains("write_on_interval_ms"))
        s.write_on_interval_ms = static_cast<int>(o.at("write_on_interval_ms").as_int64());
    if (o.if_contains("fields")) {
        for (auto&& fj : o.at("fields").as_array())
            s.fields.emplace_back(fj.as_string().c_str());
    }
    return s;
}

static SystemConfig parseSystem(const object& o) {
    SystemConfig s;
    if (o.if_contains("thread_pool_size"))
        s.thread_pool_size = static_cast<int>(o.at("thread_pool_size").as_int64());
    if (o.if_contains("log_level"))
        s.log_level = o.at("log_level").as_string().c_str();
    if (o.if_contains("log_file"))
        s.log_file = o.at("log_file").as_string().c_str();
    return s;
}

GlobalConfig JsonConfig::load(const std::string& path) {
    try {
        std::ifstream ifs(path);
        if (!ifs) throw std::runtime_error("Cannot open config file: " + path);
        const std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        value rootv = parse(content);
        object root = rootv.as_object();

        GlobalConfig cfg;
        cfg.system = parseSystem(root.at("system").as_object());
        cfg.storage = parseStorage(root.at("storage").as_object());
        for (auto&& devj : root.at("devices").as_array())
            cfg.devices.push_back(parseDevice(devj.as_object()));
        return cfg;
    } catch (const std::exception& e) {
        std::cerr << "[JsonConfig] Failed to parse config: " << e.what() << std::endl;
        throw;
    }
}
