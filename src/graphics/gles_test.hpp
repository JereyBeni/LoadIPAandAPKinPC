#pragma once

#include "platform/platform.hpp"

namespace runtime {
namespace graphics {

// Minimal SDL2 + OpenGL window + clear test.
// This is the host-side equivalent of the future GLES2 HLE path.
// Returns true if the test ran successfully (window created, context made current, clear done).
bool run_gles_clear_test(int width = 800, int height = 600, int frames = 60);

} // namespace graphics
} // namespace runtime
