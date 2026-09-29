#pragma once

#include "platform/platform.hpp"
#include <string>
#include <vector>

namespace runtime {
namespace cli {

struct Options {
    std::string command;          // inspect, elf-info, symbols, run, gles-test, help
    std::string path;             // apk / so / ipa
    std::string preferred_abi = "armeabi-v7a";
    bool verbose = false;
};

Options parse(int argc, char** argv);
void print_usage();
int run(const Options& opts);

} // namespace cli
} // namespace runtime
