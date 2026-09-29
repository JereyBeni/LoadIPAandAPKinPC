#include "cli/cli.hpp"
#include "apk/apk_inspector.hpp"
#include "elf/elf_parser.hpp"
#include "unity/unity_detector.hpp"
#include "graphics/gles_test.hpp"
#include "hle/hle_registry.hpp"
#include <iostream>

namespace runtime {
namespace cli {

void print_usage() {
    std::cout <<
R"(LoadIPAandAPKinPC v0.1.0 - HLE runtime for old Unity Android/iOS games
Targets: Windows + Linux only

Usage:
  loadipaapk <command> [options] <path>

Commands:
  inspect <game.apk|game.ipa>     Detect architecture, native libs, Unity/Mono/IL2CPP
  elf-info <lib.so>               Parse ELF headers, DT_NEEDED, basic info
  symbols <lib.so>                List symbols (imports/exports)
  gles-test                       Create SDL2 window + OpenGL clear test
  hle-dump                        Show current HLE registry
  run <game.apk>                  (stub) Future: bootstrap the game
  help                            Show this help

Examples:
  loadipaapk inspect MyTalkingTom.apk
  loadipaapk elf-info libunity.so
  loadipaapk symbols libmain.so
  loadipaapk gles-test
)" << std::endl;
}

Options parse(int argc, char** argv) {
    Options opts;
    if (argc < 2) {
        opts.command = "help";
        return opts;
    }

    opts.command = argv[1];

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--abi" && i + 1 < argc) {
            opts.preferred_abi = argv[++i];
        } else if (arg[0] != '-') {
            opts.path = arg;
        }
    }
    return opts;
}

int run(const Options& opts) {
    if (opts.command == "help" || opts.command == "--help" || opts.command == "-h") {
        print_usage();
        return 0;
    }

    if (opts.command == "gles-test") {
        bool ok = graphics::run_gles_clear_test();
        return ok ? 0 : 1;
    }

    if (opts.command == "hle-dump") {
        hle::HleRegistry::instance().dump();
        hle::HleRegistry::instance().dump_missing();
        return 0;
    }

    if (opts.path.empty()) {
        log_error("CLI", "Missing path argument");
        print_usage();
        return 1;
    }

    if (opts.command == "inspect") {
        std::string lower = opts.path;
        for (auto& c : lower) c = static_cast<char>(tolower(c));

        if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".apk") {
            auto info = apk::ApkInspector::inspect(opts.path);
            apk::ApkInspector::dump(info);

            if (info.valid) {
                auto uinfo = unity::UnityDetector::detect_from_apk(info);
                unity::UnityDetector::dump(uinfo);

                if (info.has_libunity) {
                    auto extracted = apk::ApkInspector::extract_lib(opts.path, "libunity.so", opts.preferred_abi);
                    if (extracted) {
                        log_info("Loader", "Extracted libunity.so -> " + *extracted);
                        auto elf = elf::ElfParser::parse(*extracted);
                        elf::ElfParser::dump(elf);
                    }
                }
                if (info.has_libmain) {
                    auto extracted = apk::ApkInspector::extract_lib(opts.path, "libmain.so", opts.preferred_abi);
                    if (extracted) {
                        log_info("Loader", "Extracted libmain.so -> " + *extracted);
                        auto elf = elf::ElfParser::parse(*extracted);
                        elf::ElfParser::dump(elf);
                    }
                }
            }
            return info.valid ? 0 : 1;
        } else if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".ipa") {
            log_info("IPA", "IPA inspection not fully implemented in milestone 1");
            log_info("IPA", "Will be added in next step. For now only APK is supported.");
            return 1;
        } else if (lower.size() >= 3 && lower.substr(lower.size() - 3) == ".so") {
            auto elf = elf::ElfParser::parse(opts.path);
            elf::ElfParser::dump(elf);
            auto uinfo = unity::UnityDetector::detect_from_elf(elf);
            unity::UnityDetector::dump(uinfo);
            return elf.valid ? 0 : 1;
        } else {
            log_error("CLI", "Unknown file type. Use .apk, .ipa or .so");
            return 1;
        }
    }

    if (opts.command == "elf-info") {
        auto elf = elf::ElfParser::parse(opts.path);
        elf::ElfParser::dump(elf);
        return elf.valid ? 0 : 1;
    }

    if (opts.command == "symbols") {
        auto elf = elf::ElfParser::parse(opts.path);
        if (!elf.valid) {
            log_error("ELF", elf.error);
            return 1;
        }
        log_info("ELF", "Symbols for " + opts.path);
        size_t shown = 0;
        for (const auto& s : elf.symbols) {
            if (s.name.empty()) continue;
            const char* kind = s.is_import ? "IMPORT" : (s.is_export ? "EXPORT" : "LOCAL ");
            std::cout << "  [" << kind << "] 0x" << std::hex << s.value << std::dec
                      << "  " << s.name << std::endl;
            ++shown;
            if (shown > 200) {
                std::cout << "  ... (truncated, " << elf.symbols.size() << " total)" << std::endl;
                break;
            }
        }
        return 0;
    }

    if (opts.command == "run") {
        log_info("Runtime", "run command is a stub in milestone 1");
        log_info("Runtime", "Next steps: ARM loader + Android HLE + Unity bootstrap");
        log_info("Runtime", "Philosophy: Run → Log → Detect missing API → Implement HLE → Repeat");

        auto info = apk::ApkInspector::inspect(opts.path);
        apk::ApkInspector::dump(info);
        if (!info.valid) return 1;

        auto uinfo = unity::UnityDetector::detect_from_apk(info);
        unity::UnityDetector::dump(uinfo);

        log_info("Runtime", "Would now:");
        log_info("Runtime", "  1. Extract libmain.so / libunity.so / libmono.so");
        log_info("Runtime", "  2. Load ELF with ARM interpreter / QEMU-user experimental");
        log_info("Runtime", "  3. Install HLE shims for JNI, NativeActivity, Bionic");
        log_info("Runtime", "  4. Bootstrap Unity + Mono");
        log_info("Runtime", "  5. Route GLES calls to host OpenGL via HLE");
        return 0;
    }

    log_error("CLI", "Unknown command: " + opts.command);
    print_usage();
    return 1;
}

} // namespace cli
} // namespace runtime
