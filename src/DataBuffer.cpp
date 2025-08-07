#include "DataBuffer.h"

DataBuffer& DataBuffer::instance() {
    static DataBuffer buf;
    return buf;
}

void DataBuffer::set(const std::string& varid,
                     const std::any& value,
                     const std::chrono::system_clock::time_point timestamp,
                     const VarQuality quality) {
    std::lock_guard<std::mutex> lock(mtx_);
    buffer_[varid] = {value, timestamp, quality};
}

std::any DataBuffer::get(const std::string& varid) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (const auto it = buffer_.find(varid); it != buffer_.end()) return it->second.value;
    return {};
}

DataBuffer::Entry DataBuffer::getEntry(const std::string& varid) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (const auto it = buffer_.find(varid); it != buffer_.end()) return it->second;
    return {};
}
