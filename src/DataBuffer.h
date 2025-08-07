#include "DataBuffer.h"

DataBuffer& DataBuffer::instance() {
    static DataBuffer buf;
    return buf;
}

void DataBuffer::set(const std::string& varid,
                     const std::any& value,
                     const std::chrono::system_clock::time_point timestamp,
                     const VarQuality quality) {
    std::unique_lock lock(mtx_);
    buffer_[varid] = Entry(value, timestamp, quality);
}

std::optional<std::any> DataBuffer::get(const std::string& varid) const {
    std::shared_lock lock(mtx_);
    if (const auto it = buffer_.find(varid); it != buffer_.end()) {
        return it->second.value;
    }
    return std::nullopt;
}

std::optional<DataBuffer::Entry> DataBuffer::getEntry(const std::string& varid) const {
    std::shared_lock lock(mtx_);
    if (const auto it = buffer_.find(varid); it != buffer_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void DataBuffer::remove(const std::string& varid) {
    std::unique_lock lock(mtx_);
    buffer_.erase(varid);
}
