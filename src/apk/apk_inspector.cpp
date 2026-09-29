#include "apk/apk_inspector.hpp"
#include "unity/unity_detector.hpp"
#include <zlib.h>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace runtime {
namespace apk {

#pragma pack(push, 1)
struct ZipLocalHeader {
    u32 signature;
    u16 version;
    u16 flags;
    u16 method;
    u16 mod_time;
    u16 mod_date;
    u32 crc32;
    u32 compressed_size;
    u32 uncompressed_size;
    u16 name_len;
    u16 extra_len;
};

struct ZipCentralHeader {
    u32 signature;
    u16 version_made;
    u16 version_needed;
    u16 flags;
    u16 method;
    u16 mod_time;
    u16 mod_date;
    u32 crc32;
    u32 compressed_size;
    u32 uncompressed_size;
    u16 name_len;
    u16 extra_len;
    u16 comment_len;
    u16 disk_start;
    u16 int_attr;
    u32 ext_attr;
    u32 local_offset;
};

struct ZipEndCentral {
    u32 signature;
    u16 disk_num;
    u16 disk_start;
    u16 entries_this_disk;
    u16 total_entries;
    u32 central_size;
    u32 central_offset;
    u16 comment_len;
};
#pragma pack(pop)

static bool read_file(const std::string& path, std::vector<u8>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    out.resize(sz);
    f.read(reinterpret_cast<char*>(out.data()), sz);
    return true;
}

static std::vector<std::pair<std::string, ZipCentralHeader>> list_entries(const std::vector<u8>& data) {
    std::vector<std::pair<std::string, ZipCentralHeader>> entries;
    if (data.size() < sizeof(ZipEndCentral)) return entries;

    size_t eocd_pos = 0;
    bool found = false;
    for (size_t i = data.size() - sizeof(ZipEndCentral); i > 0; --i) {
        if (data[i] == 0x50 && data[i+1] == 0x4b && data[i+2] == 0x05 && data[i+3] == 0x06) {
            eocd_pos = i;
            found = true;
            break;
        }
        if (i < 65536) break;
    }
    if (!found) return entries;

    const auto* eocd = reinterpret_cast<const ZipEndCentral*>(data.data() + eocd_pos);
    u32 central_off = eocd->central_offset;
    u16 total = eocd->total_entries;

    size_t pos = central_off;
    for (u16 i = 0; i < total; ++i) {
        if (pos + sizeof(ZipCentralHeader) > data.size()) break;
        const auto* ch = reinterpret_cast<const ZipCentralHeader*>(data.data() + pos);
        if (ch->signature != 0x02014b50) break;

        pos += sizeof(ZipCentralHeader);
        if (pos + ch->name_len > data.size()) break;

        std::string name(reinterpret_cast<const char*>(data.data() + pos), ch->name_len);
        entries.emplace_back(name, *ch);

        pos += ch->name_len + ch->extra_len + ch->comment_len;
    }
    return entries;
}

static std::vector<u8> extract_entry(const std::vector<u8>& data, const ZipCentralHeader& ch, const std::string& name) {
    size_t local_off = ch.local_offset;
    if (local_off + sizeof(ZipLocalHeader) > data.size()) return {};

    const auto* lh = reinterpret_cast<const ZipLocalHeader*>(data.data() + local_off);
    if (lh->signature != 0x04034b50) return {};

    size_t data_off = local_off + sizeof(ZipLocalHeader) + lh->name_len + lh->extra_len;
    u32 comp_size = lh->compressed_size;
    u32 uncomp_size = lh->uncompressed_size;
    u16 method = lh->method;

    if (data_off + comp_size > data.size()) return {};

    if (method == 0) {
        return std::vector<u8>(data.begin() + data_off, data.begin() + data_off + uncomp_size);
    } else if (method == 8) {
        std::vector<u8> out(uncomp_size);
        z_stream strm{};
        strm.next_in = const_cast<Bytef*>(data.data() + data_off);
        strm.avail_in = comp_size;
        strm.next_out = out.data();
        strm.avail_out = uncomp_size;

        if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) return {};
        int ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);
        if (ret != Z_STREAM_END && ret != Z_OK) return {};
        out.resize(strm.total_out);
        return out;
    }
    return {};
}

ApkInfo ApkInspector::inspect(const std::string& apk_path) {
    ApkInfo info;
    info.path = apk_path;

    std::vector<u8> data;
    if (!read_file(apk_path, data)) {
        info.error = "Cannot open APK";
        return info;
    }

    auto entries = list_entries(data);
    if (entries.empty()) {
        info.error = "Not a valid ZIP/APK or empty central directory";
        return info;
    }

    info.valid = true;

    for (const auto& [name, ch] : entries) {
        if (name.find("lib/") == 0 && name.size() > 4) {
            auto slash1 = name.find('/', 4);
            if (slash1 != std::string::npos) {
                std::string abi = name.substr(4, slash1 - 4);
                std::string libname = name.substr(slash1 + 1);

                if (libname.size() > 3 && libname.substr(libname.size() - 3) == ".so") {
                    NativeLib nl;
                    nl.name = libname;
                    nl.arch = abi;
                    nl.path_in_apk = name;
                    nl.size = ch.uncompressed_size;
                    info.native_libs.push_back(nl);

                    if (std::find(info.abis.begin(), info.abis.end(), abi) == info.abis.end()) {
                        info.abis.push_back(abi);
                    }

                    if (libname == "libmain.so")   info.has_libmain = true;
                    if (libname == "libunity.so")  info.has_libunity = true;
                    if (libname == "libmono.so" || libname.find("libmono") != std::string::npos) info.has_libmono = true;
                    if (libname == "libsqlite3.so") info.has_sqlite = true;
                    if (libname.find("SoundTouch") != std::string::npos) info.has_soundtouch = true;
                    if (libname.find("il2cpp") != std::string::npos) info.has_il2cpp = true;
                }
            }
        }

        if (name.find("assets/") == 0) {
            info.assets.push_back(name);
        }

        if (name.size() > 4 && name.substr(name.size() - 4) == ".dex") {
            info.dex_files.push_back(name);
        }

        if (name.find("assets/bin/Data/") != std::string::npos ||
            name.find("libunity.so") != std::string::npos ||
            name.find("UnityPlayer") != std::string::npos) {
            info.has_unity = true;
        }
        if (name.find("Assembly-CSharp.dll") != std::string::npos ||
            name.find("UnityEngine.dll") != std::string::npos ||
            name.find("libmono") != std::string::npos) {
            info.has_mono = true;
        }
        if (name.find("global-metadata.dat") != std::string::npos ||
            name.find("libil2cpp") != std::string::npos) {
            info.has_il2cpp = true;
        }
    }

    std::sort(info.abis.begin(), info.abis.end(), [](const std::string& a, const std::string& b) {
        if (a == "armeabi-v7a") return true;
        if (b == "armeabi-v7a") return false;
        return a < b;
    });

    return info;
}

std::vector<u8> ApkInspector::extract_file(const std::string& apk_path, const std::string& entry_name) {
    std::vector<u8> data;
    if (!read_file(apk_path, data)) return {};

    auto entries = list_entries(data);
    for (const auto& [name, ch] : entries) {
        if (name == entry_name) {
            return extract_entry(data, ch, name);
        }
    }
    return {};
}

std::optional<std::string> ApkInspector::extract_lib(const std::string& apk_path,
                                                     const std::string& lib_name,
                                                     const std::string& preferred_abi) {
    auto info = inspect(apk_path);
    if (!info.valid) return std::nullopt;

    const NativeLib* chosen = nullptr;
    for (const auto& nl : info.native_libs) {
        if (nl.name == lib_name) {
            if (nl.arch == preferred_abi) {
                chosen = &nl;
                break;
            }
            if (!chosen) chosen = &nl;
        }
    }
    if (!chosen) return std::nullopt;

    auto bytes = extract_file(apk_path, chosen->path_in_apk);
    if (bytes.empty()) return std::nullopt;

    fs::path out_dir = "runtime/extracted";
    fs::create_directories(out_dir);
    fs::path out_path = out_dir / (chosen->arch + "_" + lib_name);

    std::ofstream f(out_path, std::ios::binary);
    if (!f) return std::nullopt;
    f.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return out_path.string();
}

void ApkInspector::dump(const ApkInfo& info) {
    if (!info.valid) {
        log_error("APK", info.path + ": " + info.error);
        return;
    }

    log_info("APK", "File: " + info.path);
    log_info("APK", "Valid: yes");

    if (!info.abis.empty()) {
        std::ostringstream os;
        for (size_t i = 0; i < info.abis.size(); ++i) {
            if (i) os << ", ";
            os << info.abis[i];
        }
        log_info("APK", "ABIs: " + os.str());
        log_info("APK", "Primary architecture: " + info.abis.front());
    }

    log_info("APK", "Unity: " + std::string(info.has_unity ? "detected" : "no"));
    log_info("APK", "Mono: " + std::string(info.has_mono ? "detected" : "no"));
    log_info("APK", "IL2CPP: " + std::string(info.has_il2cpp ? "detected" : "no"));

    if (!info.native_libs.empty()) {
        log_info("APK", "Native libraries:");
        for (const auto& nl : info.native_libs) {
            std::cout << "    " << nl.name << "  [" << nl.arch << "]  "
                      << nl.size << " bytes" << std::endl;
        }
    }

    if (info.has_libmain)    log_info("APK", "  -> libmain.so present");
    if (info.has_libunity)   log_info("APK", "  -> libunity.so present");
    if (info.has_libmono)    log_info("APK", "  -> libmono present");
    if (info.has_sqlite)     log_info("APK", "  -> libsqlite3.so present");
    if (info.has_soundtouch) log_info("APK", "  -> SoundTouch plugin present");
}

} // namespace apk
} // namespace runtime
