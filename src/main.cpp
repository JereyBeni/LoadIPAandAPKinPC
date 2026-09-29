#include "cli/cli.hpp"
#include "platform/platform.hpp"

int main(int argc, char** argv) {
    runtime::log_info("Main", "LoadIPAandAPKinPC v0.1.0 - HLE runtime for old Unity mobile games");
    runtime::log_info("Main", "Targets: Windows + Linux only (macOS excluded by design)");

    auto opts = runtime::cli::parse(argc, argv);
    return runtime::cli::run(opts);
}
