#pragma once

#include "platform/platform.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace runtime {
namespace hle {

struct HleFunction {
    std::string name;
    std::string library;
    std::string original_api;
    std::string platform;
    bool implemented = false;
    std::string host_impl;
    std::string limitations;
    u64 address = 0;
};

class HleRegistry {
public:
    static HleRegistry& instance();

    void register_function(const HleFunction& fn);
    void mark_implemented(const std::string& name, const std::string& host_impl, const std::string& limitations = "");
    void mark_missing(const std::string& name, const std::string& library, u64 address = 0);

    bool is_implemented(const std::string& name) const;
    const HleFunction* get(const std::string& name) const;

    void dump() const;
    void dump_missing() const;

    size_t implemented_count() const;
    size_t missing_count() const;

private:
    HleRegistry() = default;
    std::unordered_map<std::string, HleFunction> functions_;
};

#define HLE_IMPL(name, lib, host) \
    runtime::hle::HleRegistry::instance().mark_implemented(name, host)

#define HLE_MISSING(name, lib, addr) \
    runtime::hle::HleRegistry::instance().mark_missing(name, lib, addr)

} // namespace hle
} // namespace runtime
