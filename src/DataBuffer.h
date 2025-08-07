#pragma once
#include <unordered_map>
#include <mutex>
#include <any>
#include <chrono>
#include <string>
#include "Variable.h"

class DataBuffer {
public:
    struct Entry {
        std::any value;
        std::chrono::system_clock::time_point timestamp;
        VarQuality quality = VarQuality::UNCERTAIN;
    };

    static DataBuffer& instance();

    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;

    void set(const std::string& varid,
             const std::any& value,
             std::chrono::system_clock::time_point timestamp,
             VarQuality quality);

    std::any get(const std::string& varid);

    Entry getEntry(const std::string& varid);

private:
    DataBuffer() = default;
    std::unordered_map<std::string, Entry> buffer_;
    std::mutex mtx_;
};
