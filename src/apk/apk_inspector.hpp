#pragma once

#include "platform/platform.hpp"
#include "elf/elf_parser.hpp"
#include <string>
#include <vector>
#include <optional>

namespace runtime {
namespace apk {

struct NativeLib {
    std::string name;
    std::string arch;
    std::string path_in_apk;
    size_t      size = 0;
};

struct ApkInfo {
    std::string path;
    bool        valid = false;
    std::string error;

    std::vector<std::string> abis;
    std::vector<NativeLib> native_libs;

    bool has_unity   = false;
    bool has_mono    = false;
    bool has_il2cpp  = false;
    bool has_libmain = false;
    bool has_libunity = false;
    bool has_libmono  = false;
    bool has_sqlite   = false;
    bool has_soundtouch = false;

    std::string package_name;
    std::string version_name;

    std::vector<std::string> assets;
    std::vector<std::string> dex_files;
};

class ApkInspector {
public:
    static ApkInfo inspect(const std::string& apk_path);
    static std::vector<u8> extract_file(const std::string& apk_path, const std::string& entry_name);
    static std::optional<std::string> extract_lib(const std::string& apk_path,
                                                  const std::string& lib_name,
                                                  const std::string& preferred_abi = "armeabi-v7a");
    static void dump(const ApkInfo& info);
};

} // namespace apk
} // namespace runtime
