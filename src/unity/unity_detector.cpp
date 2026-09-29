#include "unity/unity_detector.hpp"

namespace runtime {
namespace unity {

UnityInfo UnityDetector::detect_from_apk(const apk::ApkInfo& apk) {
    UnityInfo info;
    info.detected = apk.has_unity || apk.has_libunity;
    info.mono     = apk.has_mono || apk.has_libmono;
    info.il2cpp   = apk.has_il2cpp;

    for (const auto& a : apk.assets) {
        if (a.find("Assembly-CSharp.dll") != std::string::npos ||
            a.find("UnityEngine.dll") != std::string::npos ||
            a.find("Assembly-CSharp-firstpass.dll") != std::string::npos) {
            info.managed_dlls.push_back(a);
            info.mono = true;
        }
    }

    if (info.detected && info.mono) {
        info.approx_version = "Unity + Mono (pre-IL2CPP era, likely 4.x / early 5.x)";
    } else if (info.detected && info.il2cpp) {
        info.approx_version = "Unity + IL2CPP";
    } else if (info.detected) {
        info.approx_version = "Unity (backend unknown)";
    }

    return info;
}

UnityInfo UnityDetector::detect_from_elf(const elf::ElfInfo& elf) {
    UnityInfo info;
    if (!elf.valid) return info;

    for (const auto& n : elf.needed) {
        if (n.find("libunity") != std::string::npos) {
            info.detected = true;
        }
        if (n.find("mono") != std::string::npos) {
            info.mono = true;
            info.detected = true;
        }
    }

    for (const auto& s : elf.symbols) {
        if (s.name.find("Unity") != std::string::npos ||
            s.name.find("mono_") != std::string::npos) {
            info.detected = true;
            if (s.name.find("mono_") != std::string::npos) info.mono = true;
        }
    }

    if (elf.path.find("libunity") != std::string::npos) {
        info.detected = true;
    }
    if (elf.path.find("libmono") != std::string::npos) {
        info.mono = true;
        info.detected = true;
    }

    return info;
}

void UnityDetector::dump(const UnityInfo& info) {
    log_info("Unity", std::string("Detected: ") + (info.detected ? "yes" : "no"));
    if (!info.detected) return;

    log_info("Unity", std::string("Mono: ") + (info.mono ? "yes" : "no"));
    log_info("Unity", std::string("IL2CPP: ") + (info.il2cpp ? "yes" : "no"));
    if (!info.approx_version.empty()) {
        log_info("Unity", "Approx version/backend: " + info.approx_version);
    }
    if (!info.managed_dlls.empty()) {
        log_info("Unity", "Managed assemblies found:");
        for (const auto& d : info.managed_dlls) {
            std::cout << "    " << d << std::endl;
        }
    }
}

} // namespace unity
} // namespace runtime
