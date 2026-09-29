cmake_minimum_required(VERSION 3.16)
project(LoadIPAandAPKinPC VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build tests" OFF)

# Find dependencies
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# Platform defines
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
else()
    message(FATAL_ERROR "Only Windows and Linux are supported targets. macOS is explicitly excluded.")
endif()

# Sources
set(SOURCES
    src/main.cpp
    src/cli/cli.cpp
    src/apk/apk_inspector.cpp
    src/elf/elf_parser.cpp
    src/unity/unity_detector.cpp
    src/graphics/gles_test.cpp
    src/hle/hle_registry.cpp
)

add_executable(loadipaapk ${SOURCES})

target_include_directories(loadipaapk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

# Prefer imported SDL2 target (MSYS2 / modern CMake), else classic variables
if(TARGET SDL2::SDL2)
    target_link_libraries(loadipaapk PRIVATE SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(loadipaapk PRIVATE SDL2::SDL2-static)
else()
    target_link_libraries(loadipaapk PRIVATE ${SDL2_LIBRARIES})
endif()

target_link_libraries(loadipaapk PRIVATE
    OpenGL::GL
    ZLIB::ZLIB
    Threads::Threads
)

# Windows specific
if(WIN32)
    target_link_libraries(loadipaapk PRIVATE opengl32)
endif()

# Install
install(TARGETS loadipaapk RUNTIME DESTINATION bin)

message(STATUS "LoadIPAandAPKinPC configured")
message(STATUS "  Targets: Windows + Linux only")
message(STATUS "  First milestone: APK inspect + ELF + Unity detect + SDL2/GL test + HLE registry")
