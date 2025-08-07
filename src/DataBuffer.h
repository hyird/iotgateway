#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <any>
#include <chrono>
#include <string>
#include <optional>
#include "Variable.h"

class DataBuffer {
public:
    struct Entry {
        std::any value{};
        std::chrono::system_clock::time_point timestamp{};
        VarQuality quality = VarQuality::UNCERTAIN;

        Entry() = default;
        Entry(const std::any& v,
              const std::chrono::system_clock::time_point& t,
              VarQuality q)
            : value(v), timestamp(t), quality(q) {}
    };

    static DataBuffer& instance();

    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;
    ~DataBuffer() = default;

    void set(const std::string& varid,
             const std::any& value,
             std::chrono::system_clock::time_point timestamp,
             VarQuality quality);

    std::optional<std::any> get(const std::string& varid) const;

    std::optional<Entry> getEntry(const std::string& varid) const;

    void remove(const std::string& varid);

private:
    explicit DataBuffer() = default;

    std::unordered_map<std::string, Entry> buffer_;
    mutable std::shared_mutex mtx_;
};
