#pragma once

#include "platform/platform.hpp"
#include <string>
#include <vector>
#include <optional>

namespace runtime {
namespace elf {

enum class ElfClass { None, ELF32, ELF64 };
enum class ElfArch  { Unknown, ARM, ARM64, X86, X86_64 };

struct ElfHeaderInfo {
    ElfClass  cls       = ElfClass::None;
    ElfArch   arch      = ElfArch::Unknown;
    u16       type      = 0;
    u16       machine   = 0;
    u32       version   = 0;
    u64       entry     = 0;
    u64       phoff     = 0;
    u64       shoff     = 0;
    u16       phnum     = 0;
    u16       shnum     = 0;
    bool      is_shared = false;
    bool      is_pie    = false;
};

struct DynamicEntry {
    s64 tag = 0;
    u64 val = 0;
    std::string str;
};

struct Symbol {
    std::string name;
    u64         value = 0;
    u64         size  = 0;
    u8          info  = 0;
    u8          other = 0;
    u16         shndx = 0;
    bool        is_import = false;
    bool        is_export = false;
};

struct ElfInfo {
    std::string               path;
    ElfHeaderInfo             header;
    std::vector<std::string>  needed;
    std::vector<DynamicEntry> dynamic;
    std::vector<Symbol>       symbols;
    std::vector<std::string>  section_names;
    bool                      valid = false;
    std::string               error;
};

class ElfParser {
public:
    static ElfInfo parse(const std::string& path);
    static ElfInfo parse(const u8* data, size_t size, const std::string& name = "<buffer>");
    static void dump(const ElfInfo& info);

private:
    static bool parse_header(const u8* data, size_t size, ElfHeaderInfo& out, std::string& err);
    static bool parse_dynamic(const u8* data, size_t size, const ElfHeaderInfo& hdr, ElfInfo& out);
    static bool parse_symbols(const u8* data, size_t size, const ElfHeaderInfo& hdr, ElfInfo& out);
};

} // namespace elf
} // namespace runtime
