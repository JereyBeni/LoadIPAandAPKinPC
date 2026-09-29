#include "hle/hle_registry.hpp"
#include <iostream>

namespace runtime {
namespace hle {

HleRegistry& HleRegistry::instance() {
    static HleRegistry reg;
    return reg;
}

void HleRegistry::register_function(const HleFunction& fn) {
    functions_[fn.name] = fn;
}

void HleRegistry::mark_implemented(const std::string& name, const std::string& host_impl, const std::string& limitations) {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        it->second.implemented = true;
        it->second.host_impl = host_impl;
        it->second.limitations = limitations;
    } else {
        HleFunction fn;
        fn.name = name;
        fn.implemented = true;
        fn.host_impl = host_impl;
        fn.limitations = limitations;
        functions_[name] = fn;
    }
    log_info("HLE", name + " -> implemented (" + host_impl + ")");
}

void HleRegistry::mark_missing(const std::string& name, const std::string& library, u64 address) {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        it->second.implemented = false;
        it->second.library = library;
        it->second.address = address;
    } else {
        HleFunction fn;
        fn.name = name;
        fn.library = library;
        fn.address = address;
        fn.implemented = false;
        functions_[name] = fn;
    }
    log_missing(library, name, address);
}

bool HleRegistry::is_implemented(const std::string& name) const {
    auto it = functions_.find(name);
    return it != functions_.end() && it->second.implemented;
}

const HleFunction* HleRegistry::get(const std::string& name) const {
    auto it = functions_.find(name);
    return it != functions_.end() ? &it->second : nullptr;
}

void HleRegistry::dump() const {
    log_info("HLE", "=== HLE Registry Dump ===");
    for (const auto& [name, fn] : functions_) {
        std::cout << "  " << (fn.implemented ? "[OK] " : "[MISSING] ")
                  << name << "  (lib: " << fn.library << ")"
                  << (fn.address ? " @ 0x" + std::to_string(fn.address) : "")
                  << std::endl;
        if (fn.implemented && !fn.host_impl.empty()) {
            std::cout << "       host: " << fn.host_impl << std::endl;
        }
        if (!fn.limitations.empty()) {
            std::cout << "       limitations: " << fn.limitations << std::endl;
        }
    }
}

void HleRegistry::dump_missing() const {
    log_info("HLE", "=== Missing HLE functions ===");
    size_t count = 0;
    for (const auto& [name, fn] : functions_) {
        if (!fn.implemented) {
            std::cout << "  [HLE] MISSING: " << name << std::endl;
            std::cout << "  [HLE] Library: " << fn.library << std::endl;
            if (fn.address) {
                std::cout << "  [HLE] Address: 0x" << std::hex << fn.address << std::dec << std::endl;
            }
            ++count;
        }
    }
    if (count == 0) {
        log_info("HLE", "No missing functions registered yet.");
    }
}

size_t HleRegistry::implemented_count() const {
    size_t n = 0;
    for (const auto& [_, fn] : functions_) if (fn.implemented) ++n;
    return n;
}

size_t HleRegistry::missing_count() const {
    size_t n = 0;
    for (const auto& [_, fn] : functions_) if (!fn.implemented) ++n;
    return n;
}

} // namespace hle
} // namespace runtime
