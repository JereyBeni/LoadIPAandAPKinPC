#include "elf/elf_parser.hpp"
#include <cstring>
#include <fstream>
#include <sstream>

namespace runtime {
namespace elf {

static constexpr u8  EI_CLASS = 4;
static constexpr u8  EI_DATA  = 5;
static constexpr u8  ELFCLASS32 = 1;
static constexpr u8  ELFCLASS64 = 2;
static constexpr u8  ELFDATA2LSB = 1;

static constexpr u16 ET_DYN  = 3;
static constexpr u16 EM_ARM     = 40;
static constexpr u16 EM_AARCH64 = 183;
static constexpr u16 EM_386     = 3;
static constexpr u16 EM_X86_64  = 62;

static constexpr s32 DT_NULL   = 0;
static constexpr s32 DT_NEEDED = 1;
static constexpr s32 DT_STRTAB = 5;
static constexpr s32 DT_SYMTAB = 6;
static constexpr s32 DT_STRSZ  = 10;
static constexpr s32 DT_SYMENT = 11;
static constexpr s32 DT_SONAME = 14;
static constexpr s32 DT_RPATH  = 15;

static constexpr u32 PT_LOAD    = 1;
static constexpr u32 PT_DYNAMIC = 2;

#pragma pack(push, 1)
struct Elf32_Ehdr {
    u8 e_ident[16]; u16 e_type; u16 e_machine; u32 e_version;
    u32 e_entry; u32 e_phoff; u32 e_shoff; u32 e_flags;
    u16 e_ehsize; u16 e_phentsize; u16 e_phnum; u16 e_shentsize; u16 e_shnum; u16 e_shstrndx;
};
struct Elf64_Ehdr {
    u8 e_ident[16]; u16 e_type; u16 e_machine; u32 e_version;
    u64 e_entry; u64 e_phoff; u64 e_shoff; u32 e_flags;
    u16 e_ehsize; u16 e_phentsize; u16 e_phnum; u16 e_shentsize; u16 e_shnum; u16 e_shstrndx;
};
struct Elf32_Phdr {
    u32 p_type; u32 p_offset; u32 p_vaddr; u32 p_paddr;
    u32 p_filesz; u32 p_memsz; u32 p_flags; u32 p_align;
};
struct Elf64_Phdr {
    u32 p_type; u32 p_flags; u64 p_offset; u64 p_vaddr;
    u64 p_paddr; u64 p_filesz; u64 p_memsz; u64 p_align;
};
struct Elf32_Dyn { s32 d_tag; u32 d_val; };
struct Elf64_Dyn { s64 d_tag; u64 d_val; };
struct Elf32_Sym {
    u32 st_name; u32 st_value; u32 st_size; u8 st_info; u8 st_other; u16 st_shndx;
};
struct Elf64_Sym {
    u32 st_name; u8 st_info; u8 st_other; u16 st_shndx; u64 st_value; u64 st_size;
};
#pragma pack(pop)

bool ElfParser::parse_header(const u8* data, size_t size, ElfHeaderInfo& out, std::string& err) {
    if (size < 16) { err = "File too small for ELF header"; return false; }
    if (data[0] != 0x7F || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
        err = "Not an ELF file (bad magic)"; return false;
    }
    if (data[EI_DATA] != ELFDATA2LSB) { err = "Only little-endian ELF supported"; return false; }

    u8 cls = data[EI_CLASS];
    if (cls == ELFCLASS32) {
        if (size < sizeof(Elf32_Ehdr)) { err = "Truncated ELF32 header"; return false; }
        const auto* h = reinterpret_cast<const Elf32_Ehdr*>(data);
        out.cls = ElfClass::ELF32; out.type = h->e_type; out.machine = h->e_machine;
        out.version = h->e_version; out.entry = h->e_entry; out.phoff = h->e_phoff;
        out.shoff = h->e_shoff; out.phnum = h->e_phnum; out.shnum = h->e_shnum;
        out.is_shared = (h->e_type == ET_DYN);
    } else if (cls == ELFCLASS64) {
        if (size < sizeof(Elf64_Ehdr)) { err = "Truncated ELF64 header"; return false; }
        const auto* h = reinterpret_cast<const Elf64_Ehdr*>(data);
        out.cls = ElfClass::ELF64; out.type = h->e_type; out.machine = h->e_machine;
        out.version = h->e_version; out.entry = h->e_entry; out.phoff = h->e_phoff;
        out.shoff = h->e_shoff; out.phnum = h->e_phnum; out.shnum = h->e_shnum;
        out.is_shared = (h->e_type == ET_DYN);
    } else {
        err = "Unknown ELF class"; return false;
    }

    switch (out.machine) {
        case EM_ARM:     out.arch = ElfArch::ARM; break;
        case EM_AARCH64: out.arch = ElfArch::ARM64; break;
        case EM_386:     out.arch = ElfArch::X86; break;
        case EM_X86_64:  out.arch = ElfArch::X86_64; break;
        default:         out.arch = ElfArch::Unknown; break;
    }
    return true;
}

static u64 vaddr_to_offset(const u8* data, size_t size, const ElfHeaderInfo& hdr, u64 vaddr) {
    if (hdr.cls == ElfClass::ELF32) {
        for (u16 i = 0; i < hdr.phnum; ++i) {
            u64 off = hdr.phoff + i * sizeof(Elf32_Phdr);
            if (off + sizeof(Elf32_Phdr) > size) break;
            const auto* ph = reinterpret_cast<const Elf32_Phdr*>(data + off);
            if (ph->p_type == PT_LOAD && vaddr >= ph->p_vaddr && vaddr < ph->p_vaddr + ph->p_filesz)
                return ph->p_offset + (vaddr - ph->p_vaddr);
        }
    } else {
        for (u16 i = 0; i < hdr.phnum; ++i) {
            u64 off = hdr.phoff + i * sizeof(Elf64_Phdr);
            if (off + sizeof(Elf64_Phdr) > size) break;
            const auto* ph = reinterpret_cast<const Elf64_Phdr*>(data + off);
            if (ph->p_type == PT_LOAD && vaddr >= ph->p_vaddr && vaddr < ph->p_vaddr + ph->p_filesz)
                return ph->p_offset + (vaddr - ph->p_vaddr);
        }
    }
    return vaddr;
}

bool ElfParser::parse_dynamic(const u8* data, size_t size, const ElfHeaderInfo& hdr, ElfInfo& out) {
    u64 dyn_offset = 0, dyn_size = 0;
    if (hdr.cls == ElfClass::ELF32) {
        for (u16 i = 0; i < hdr.phnum; ++i) {
            u64 off = hdr.phoff + i * sizeof(Elf32_Phdr);
            if (off + sizeof(Elf32_Phdr) > size) break;
            const auto* ph = reinterpret_cast<const Elf32_Phdr*>(data + off);
            if (ph->p_type == PT_DYNAMIC) { dyn_offset = ph->p_offset; dyn_size = ph->p_filesz; break; }
        }
    } else {
        for (u16 i = 0; i < hdr.phnum; ++i) {
            u64 off = hdr.phoff + i * sizeof(Elf64_Phdr);
            if (off + sizeof(Elf64_Phdr) > size) break;
            const auto* ph = reinterpret_cast<const Elf64_Phdr*>(data + off);
            if (ph->p_type == PT_DYNAMIC) { dyn_offset = ph->p_offset; dyn_size = ph->p_filesz; break; }
        }
    }
    if (dyn_offset == 0 || dyn_size == 0) return true;

    u64 strtab = 0, strsz = 0;
    std::vector<DynamicEntry> entries;

    if (hdr.cls == ElfClass::ELF32) {
        size_t count = dyn_size / sizeof(Elf32_Dyn);
        for (size_t i = 0; i < count; ++i) {
            u64 off = dyn_offset + i * sizeof(Elf32_Dyn);
            if (off + sizeof(Elf32_Dyn) > size) break;
            const auto* d = reinterpret_cast<const Elf32_Dyn*>(data + off);
            DynamicEntry e; e.tag = d->d_tag; e.val = d->d_val;
            entries.push_back(e);
            if (d->d_tag == DT_STRTAB) strtab = d->d_val;
            if (d->d_tag == DT_STRSZ)  strsz  = d->d_val;
            if (d->d_tag == DT_NULL) break;
        }
    } else {
        size_t count = dyn_size / sizeof(Elf64_Dyn);
        for (size_t i = 0; i < count; ++i) {
            u64 off = dyn_offset + i * sizeof(Elf64_Dyn);
            if (off + sizeof(Elf64_Dyn) > size) break;
            const auto* d = reinterpret_cast<const Elf64_Dyn*>(data + off);
            DynamicEntry e; e.tag = d->d_tag; e.val = d->d_val;
            entries.push_back(e);
            if (d->d_tag == DT_STRTAB) strtab = d->d_val;
            if (d->d_tag == DT_STRSZ)  strsz  = d->d_val;
            if (d->d_tag == DT_NULL) break;
        }
    }

    u64 strtab_off = vaddr_to_offset(data, size, hdr, strtab);
    for (auto& e : entries) {
        if (e.tag == DT_NEEDED || e.tag == DT_SONAME || e.tag == DT_RPATH) {
            u64 soff = strtab_off + e.val;
            if (soff < size) {
                const char* s = reinterpret_cast<const char*>(data + soff);
                size_t max = size - soff, len = 0;
                while (len < max && s[len]) ++len;
                e.str = std::string(s, len);
            }
        }
        if (e.tag == DT_NEEDED && !e.str.empty()) out.needed.push_back(e.str);
    }
    out.dynamic = std::move(entries);
    return true;
}

bool ElfParser::parse_symbols(const u8* data, size_t size, const ElfHeaderInfo& hdr, ElfInfo& out) {
    u64 symtab = 0, strtab = 0, syment = 0, strsz = 0;
    for (const auto& e : out.dynamic) {
        if (e.tag == DT_SYMTAB) symtab = e.val;
        if (e.tag == DT_STRTAB) strtab = e.val;
        if (e.tag == DT_SYMENT) syment = e.val;
        if (e.tag == DT_STRSZ)  strsz  = e.val;
    }
    if (symtab == 0 || strtab == 0 || syment == 0) return true;

    u64 sym_off = vaddr_to_offset(data, size, hdr, symtab);
    u64 str_off = vaddr_to_offset(data, size, hdr, strtab);

    for (size_t i = 0; i < 4096; ++i) {
        u64 off = sym_off + i * syment;
        if (off + syment > size) break;

        Symbol sym;
        if (hdr.cls == ElfClass::ELF32) {
            const auto* s = reinterpret_cast<const Elf32_Sym*>(data + off);
            sym.value = s->st_value; sym.size = s->st_size; sym.info = s->st_info;
            sym.other = s->st_other; sym.shndx = s->st_shndx;
            u32 name_off = s->st_name;
            if (str_off + name_off < size) {
                const char* n = reinterpret_cast<const char*>(data + str_off + name_off);
                size_t max = size - (str_off + name_off), len = 0;
                while (len < max && n[len]) ++len;
                sym.name = std::string(n, len);
            }
        } else {
            const auto* s = reinterpret_cast<const Elf64_Sym*>(data + off);
            sym.value = s->st_value; sym.size = s->st_size; sym.info = s->st_info;
            sym.other = s->st_other; sym.shndx = s->st_shndx;
            u32 name_off = s->st_name;
            if (str_off + name_off < size) {
                const char* n = reinterpret_cast<const char*>(data + str_off + name_off);
                size_t max = size - (str_off + name_off), len = 0;
                while (len < max && n[len]) ++len;
                sym.name = std::string(n, len);
            }
        }

        if (i > 0 && sym.name.empty() && sym.value == 0 && sym.size == 0) break;

        u8 bind = (sym.info >> 4) & 0xF;
        if (sym.shndx == 0 && !sym.name.empty()) sym.is_import = true;
        else if (sym.shndx != 0 && !sym.name.empty() && (bind == 1 || bind == 2)) sym.is_export = true;

        if (!sym.name.empty()) out.symbols.push_back(std::move(sym));
    }
    return true;
}

ElfInfo ElfParser::parse(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        ElfInfo info; info.path = path; info.error = "Cannot open file"; return info;
    }
    auto size = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<u8> buf(size);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return parse(buf.data(), size, path);
}

ElfInfo ElfParser::parse(const u8* data, size_t size, const std::string& name) {
    ElfInfo info;
    info.path = name;
    if (!parse_header(data, size, info.header, info.error)) return info;
    parse_dynamic(data, size, info.header, info);
    parse_symbols(data, size, info.header, info);
    info.valid = true;
    return info;
}

void ElfParser::dump(const ElfInfo& info) {
    if (!info.valid) {
        log_error("ELF", info.path + ": " + info.error);
        return;
    }
    log_info("ELF", "File: " + info.path);
    const char* cls = info.header.cls == ElfClass::ELF32 ? "ELF32" :
                      info.header.cls == ElfClass::ELF64 ? "ELF64" : "???";
    const char* arch = "Unknown";
    switch (info.header.arch) {
        case ElfArch::ARM:    arch = "ARMv7"; break;
        case ElfArch::ARM64:  arch = "ARM64"; break;
        case ElfArch::X86:    arch = "x86"; break;
        case ElfArch::X86_64: arch = "x86_64"; break;
        default: break;
    }
    log_info("ELF", std::string("Architecture: ") + arch + " (" + cls + ")");
    log_info("ELF", std::string("Type: ") + (info.header.is_shared ? "Shared object (ET_DYN)" : "Executable/other"));
    {
        std::ostringstream os; os << std::hex << info.header.entry;
        log_info("ELF", "Entry: 0x" + os.str());
    }
    if (!info.needed.empty()) {
        for (const auto& n : info.needed) log_info("ELF", "Needed: " + n);
    } else {
        log_info("ELF", "Needed: (none or not found)");
    }
    size_t imports = 0, exports = 0;
    for (const auto& s : info.symbols) {
        if (s.is_import) ++imports;
        if (s.is_export) ++exports;
    }
    log_info("ELF", "Symbols: " + std::to_string(info.symbols.size()) +
                     " (imports: " + std::to_string(imports) +
                     ", exports: " + std::to_string(exports) + ")");
}

} // namespace elf
} // namespace runtime
