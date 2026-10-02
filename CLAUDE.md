# CLAUDE.md

Desktop port and modding project for the Android release of *Call of Duty: Black Ops Zombies* (Ideaworks, Marmalade SDK 10). The game logic is 32-bit ARM code inside `boz.s3e`; `runtime/` runs it on x86 Linux.

## How the runtime works

`runtime/` is a fork of [cod-boz-port](https://github.com/Producdevity/cod-boz-port) (MIT, fork point in `runtime/UPSTREAM_COMMIT`). Upstream runs the ARM code natively on ARM handhelds and implements the Marmalade (s3e) API in C. The fork adds:

- `src/arm_emu.c`: Unicorn-based ARM CPU. The loader is built as 32-bit x86 so guest and host share addresses. Host memory is identity-mapped into the emulator on first touch (whole `/proc/self/maps` regions). Any guest jump into host code lands on a shadow page of `svc` instructions; the trap calls that x86 function with r0-r3 plus stack words and returns r0:r1. No thunk table.
- `s3e_guest_call()` (in `s3e_host.c`): every place the C code calls back into game code goes through it.
- Desktop input in `s3e_input.c`: mouse pointer for menus, Tab toggles game mode (WASD on touchpad 0 stick, mouse look on touchpad 1 stick centered at x=768, keys mapped to Xperia Play keys).
- Window scaling in `s3e_gl.c`: game renders to a 1280x720 FBO, blitted to the window each swap. Super+W/close maps to `s3eDeviceRequestQuit`.

ARM `#if defined(__arm__)` paths are left in the source but nothing builds or ships them.

## Layout

- `runtime/`: the desktop runtime. Build with CMake presets: `cmake --preset linux-x86`, `cmake --build --preset linux-x86` (`linux-x86-tests` builds tests too), `ctest --preset linux-x86`. Outputs go to `build/linux-x86/bin/` (loader, extractor, `libSDL2`; loader RUNPATH is `$ORIGIN`). Then `scripts/setup-game.sh`, `scripts/run-desktop.sh`.
- Windows: `cmake --preset windows-x86 && cmake --build --preset windows-x86 && scripts/package-windows.sh` (MinGW-w64 cross build; bundles Mesa from mesa-dist-win). Windows renders through Mesa's WGL `opengl32.dll`: SDL makes a desktop GL window and `s3e_egl.c` swaps in an OpenGL ES context via `WGL_EXT_create_context_es_profile` (Mesa's EGL can't create window surfaces on Windows). `src/platform/gl_probe_win32.c` picks `GALLIUM_DRIVER` at startup (d3d12, else llvmpipe). Linux tarball: `scripts/package-linux.sh`. CI: `.github/workflows/build.yml`; tags `v*` publish a release.
- `runtime/third_party/patches/unicorn-*.patch`: applied to the Unicorn submodule at configure time (TLB/dirty-tracking speedups, Windows code-buffer commit fix).
- `runtime/third_party/unicorn`, `runtime/third_party/SDL2`: submodules, built 32-bit as CMake subprojects via `cmake/i686-linux.cmake` (the `gcc-m32` wrapper is required; Unicorn's CMake ignores `-m32` flags).
- `tools/destin`: submodule for `dade`, the `.dz` asset extractor. Install with `uv tool install dade`. Always pass `--no-delete` to `dade marmalade extract-dz`; it deletes the source archive by default.
- `original/` (gitignored): `com.activision.boz.apk` (1.0.11) and `obb/` with the CDN packs plus `.dz.dat` markers.
- `assets/` (gitignored): extracted game data and pack links, made by `setup-game.sh`. The repo root is the game root (`--root`); the port looks up files in `<root>/assets/`.
- `saves/` (gitignored): game saves and `device-id.bin`. `run-desktop.sh` sets `HOME` to it (override with `BOZ_SAVES`) and keeps `XDG_CACHE_HOME` on the real cache.

Never commit game files (`*.apk`, `*.dz`, extracted assets).

## Game facts

- Target version 1.0.11 (versionCode 1045111). Packs come from `http://cdn-boz-android.callofduty.com/PROD/CODBOZ/1_0_9/`. The runtime needs `blackops_etc.dz` and `blackops_gles1.dz`; upstream setup expects SHA-256 `670cefce...` and `60846cf7...`.
- The local APK is genuine game payload but a re-signed repack (adds `com.savegame.SavesRestoring` and `assets/su.save`, a premade save). The extractor skips `su.save`.
- Image loads at `0x4a000000`. Game `memcpy` is at image offset `0x366000`.
- Upstream's ARM crash-recovery hacks in `main.c` (fixed game offsets) are not yet ported to the emulator fault handler.

## Debugging

- `BOZ_TRACE_STATUS=1`: status line every 2 s (host call rate, last host function, main ARM pc/lr).
- `BOZ_TRACE_CALLS=N`: log the first N host calls with arguments.
- Emulator faults print `[arm] ... pc= lr=` with all registers.
- Other env: `BOZ_DISPLAY=WxH`, `BOZ_STRETCH=1`, `BOZ_NO_SCALE=1`, `BOZ_MOUSE_SENS` (default 12000), `BOZ_LOOK_RADIUS`, `BOZ_LOOK_MODE=swipe`.
- Hyprland here uses Lua dispatchers: `hyprctl dispatch 'hl.dsp.focus({ window = "class:codboz_s3e_loader" })'`.
- Ghidra MCP: configured in a local, gitignored `.mcp.json` pointing at a ghidra-mcp clone. It only answers while Ghidra is open with a program loaded.

## Working with the user

- The user runs the game and reports what they see; don't launch windows or change their desktop unless asked.
- Building needs 32-bit libs: `lib32-glibc`, `lib32-mesa`, `lib32-libglvnd`, `lib32-libxkbcommon`, `lib32-libdecor`. SDL2 is built from the submodule, not installed.
