#pragma once

#include "platform/platform.hpp"
#include "apk/apk_inspector.hpp"
#include "elf/elf_parser.hpp"
#include <string>

namespace runtime {
namespace unity {

struct UnityInfo {
    bool detected = false;
    bool mono = false;
    bool il2cpp = false;
    std::string approx_version;     // best-effort from strings or assets
    std::string engine_path;        // libunity.so path if extracted
    std::vector<std::string> managed_dlls; // Assembly-CSharp.dll etc. if found in assets
};

class UnityDetector {
public:
    static UnityInfo detect_from_apk(const apk::ApkInfo& apk);
    static UnityInfo detect_from_elf(const elf::ElfInfo& elf);

    static void dump(const UnityInfo& info);
};

} // namespace unity
} // namespace runtime
