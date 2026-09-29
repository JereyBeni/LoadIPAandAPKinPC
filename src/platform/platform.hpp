#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
#else
    #error "Only Windows and Linux are supported. macOS is excluded by design."
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>

namespace runtime {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s32 = int32_t;
using s64 = int64_t;

inline void log_info(const std::string& tag, const std::string& msg) {
    std::cout << "[" << tag << "] " << msg << std::endl;
}

inline void log_warn(const std::string& tag, const std::string& msg) {
    std::cerr << "[" << tag << "] WARN: " << msg << std::endl;
}

inline void log_error(const std::string& tag, const std::string& msg) {
    std::cerr << "[" << tag << "] ERROR: " << msg << std::endl;
}

inline void log_missing(const std::string& lib, const std::string& func, u64 addr = 0) {
    std::cerr << "[HLE] MISSING: " << func << std::endl;
    std::cerr << "[HLE] Library: " << lib << std::endl;
    if (addr) {
        std::cerr << "[HLE] Address: 0x" << std::hex << addr << std::dec << std::endl;
    }
}

} // namespace runtime
