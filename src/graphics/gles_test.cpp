#include "graphics/gles_test.hpp"
#include "hle/hle_registry.hpp"

#include <SDL.h>
#include <SDL_opengl.h>

namespace runtime {
namespace graphics {

bool run_gles_clear_test(int width, int height, int frames) {
    log_info("Graphics", "Starting SDL2 + OpenGL clear test...");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        log_error("Graphics", std::string("SDL_Init failed: ") + SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow(
        "LoadIPAandAPKinPC - GLES2 HLE Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (!window) {
        log_error("Graphics", std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        log_error("Graphics", std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    log_info("Graphics", "Window + OpenGL context created");
    log_info("Graphics", "This is the host backend that future GLES2 HLE will target");

    auto& reg = hle::HleRegistry::instance();
    reg.register_function({"eglGetDisplay", "libEGL.so", "eglGetDisplay", "Android", true, "SDL2 window + host GL", ""});
    reg.register_function({"eglInitialize", "libEGL.so", "eglInitialize", "Android", true, "no-op / SDL already init", ""});
    reg.register_function({"eglChooseConfig", "libEGL.so", "eglChooseConfig", "Android", true, "SDL_GL_SetAttribute", ""});
    reg.register_function({"eglCreateContext", "libEGL.so", "eglCreateContext", "Android", true, "SDL_GL_CreateContext", ""});
    reg.register_function({"eglCreateWindowSurface", "libEGL.so", "eglCreateWindowSurface", "Android", true, "SDL_Window", ""});
    reg.register_function({"eglMakeCurrent", "libEGL.so", "eglMakeCurrent", "Android", true, "SDL_GL_MakeCurrent", ""});
    reg.register_function({"eglSwapBuffers", "libEGL.so", "eglSwapBuffers", "Android", true, "SDL_GL_SwapWindow", ""});
    reg.register_function({"glClear", "libGLESv2.so", "glClear", "Android", true, "host glClear", ""});
    reg.register_function({"glClearColor", "libGLESv2.so", "glClearColor", "Android", true, "host glClearColor", ""});
    reg.register_function({"glViewport", "libGLESv2.so", "glViewport", "Android", true, "host glViewport", ""});

    HLE_IMPL("eglGetDisplay", "libEGL.so", "SDL2");
    HLE_IMPL("glClear", "libGLESv2.so", "host OpenGL");

    for (int i = 0; i < frames; ++i) {
        float t = static_cast<float>(i) / frames;
        glClearColor(0.1f + 0.3f * t, 0.15f, 0.25f + 0.2f * (1.0f - t), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                frames = i;
                break;
            }
        }
        SDL_Delay(16);
    }

    log_info("Graphics", "Clear test finished successfully");

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

} // namespace graphics
} // namespace runtime
