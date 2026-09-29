# LoadIPAandAPKinPC

**HLE / compatibility runtime** for old Unity games originally built for Android (APK) and iOS (IPA).

Philosophy:

```
Run → Log → Detect missing API → Implement HLE → Run again → Repeat
```

Inspired conceptually by projects like TouchHLE: execute original code when possible, provide compatible implementations of the APIs the game actually needs.

## Targets

- **Windows 10/11**
- **Linux**

macOS is **explicitly out of scope**. No Metal, no Cocoa, no Xcode.

WSL is only a development/tooling environment on Windows, not a third target.

## First game target

**My Talking Tom** (Outfit7) – old Android version from November 2013.

- Unity + Mono
- ARMv7 libraries: `libmain.so`, `libunity.so`, `libmono.so`, `libsqlite3.so`, `libSoundTouchPlugin.so`

## Current status – Milestone 1

Working:

- Project builds on Windows / Linux with CMake
- CLI: `inspect`, `elf-info`, `symbols`, `gles-test`, `hle-dump`, `run` (stub)
- APK inspection (ABI, native libs, Unity/Mono/IL2CPP detection)
- Basic ELF32/ELF64 parser (headers, DT_NEEDED, symbols)
- Unity detector
- SDL2 + OpenGL clear test (host backend for future GLES2 HLE)
- HLE registry (register / mark missing / dump)

Not yet:

- ARM code execution / loader
- Android HLE (JNI, NativeActivity, Bionic shims)
- Real Unity/Mono bootstrap
- Full GLES2 HLE
- Audio / Input / Filesystem sandbox
- IPA support beyond stub

## Build

### Dependencies

- CMake ≥ 3.16
- C++17 compiler (MSVC, GCC, Clang)
- SDL2
- OpenGL
- zlib
- pthread (Linux)

### Fedora / RHEL

```bash
sudo dnf install gcc-c++ cmake SDL2-devel mesa-libGL-devel zlib-devel
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Debian / Ubuntu

```bash
sudo apt install build-essential cmake libsdl2-dev libgl1-mesa-dev zlib1g-dev
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows (MSVC or MinGW)

```bash
mkdir build && cd build
cmake .. -DSDL2_DIR=C:/path/to/SDL2
cmake --build . --config Release
```

## Usage examples

```bash
./loadipaapk inspect MyTalkingTom.apk
./loadipaapk elf-info libunity.so
./loadipaapk symbols libmain.so
./loadipaapk gles-test
./loadipaapk hle-dump
./loadipaapk run MyTalkingTom.apk
```

## Next milestones

1. ARM ELF loader (map segments, relocations, PLT/GOT stubs)
2. Minimal Android HLE (`__android_log_print`, `dlopen`/`dlsym`, `JNI_OnLoad`, etc.)
3. Experimental ARM execution path
4. Unity/Mono bootstrap for 2013-era games
5. GLES2 → host OpenGL translation
6. Filesystem sandbox + input mapping

## Rules

- Do **not** remake the game.
- Do **not** implement APIs that are not requested by the binary or by the runtime log.
- Every HLE function must document: original API, platform, library, expected behaviour, host implementation, limitations.
- Windows + Linux only.
