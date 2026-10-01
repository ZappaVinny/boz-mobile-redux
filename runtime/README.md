# BOZ desktop runtime

Runs the Android Marmalade build of *Call of Duty: Black Ops Zombies* (1.0.11) on x86 Linux. Forked from [cod-boz-port](https://github.com/Producdevity/cod-boz-port) at the commit in `UPSTREAM_COMMIT`. Upstream runs the game's ARM code natively on ARM handhelds; this fork runs it through an ARM CPU emulator.

## Build

```bash
make          # Unicorn + SDL2 (32-bit, from third_party/), the loader, the APK extractor
make test     # host unit tests (32-bit) and the ARM bridge test
make clean
```

Outputs: `build/x86/codboz_s3e_loader`, `build/host/codboz_apk_extract`, `build/x86/sdl2-install/lib/`.

## Run

```bash
scripts/setup-game.sh [apk] [packs-dir] [game-dir]
scripts/run-desktop.sh [game-dir]
```

`run-desktop.sh` uses the bundled SDL2, picks Wayland when the 32-bit `libxkbcommon` is installed (X11 otherwise), and opens a resizable 1280x720 window.

## Design

- **32-bit host, shared address space.** The loader is built with `-m32`, so the game and the C runtime use the same pointers. `arm_emu.c` maps host memory into Unicorn on first access, one `/proc/self/maps` region at a time.
- **Calls into the runtime.** When guest code jumps to a host function, the fetch hits a shadow page filled with `svc` instructions. The trap handler calls the x86 function at that address with r0-r3 and stack arguments, writes r0/r1 back and returns to lr. Imports, extension tables and GL proc addresses all work this way.
- **Calls into the game.** `s3e_guest_call()` runs guest code on the calling thread's emulated CPU. Each host thread gets its own CPU; nested calls save and restore context.
- **Rendering.** The game draws into a 1280x720 framebuffer object that is blitted to the window on every swap. GLES 1 functions resolve through EGL when no `libGLESv1_CM` exists.
- **Input.** SDL keyboard and mouse state drive the s3e pointer in menu mode and the Xperia Play touchpads and keys in game mode (Tab toggles).

## Environment

| Variable | Effect |
| --- | --- |
| `BOZ_DISPLAY=WxH` | Game resolution (default 1280x720) |
| `BOZ_STRETCH=1` | Fill the window instead of keeping 16:9 |
| `BOZ_NO_SCALE=1` | Disable the scaling framebuffer |
| `BOZ_WINDOWED=0` | Start fullscreen |
| `BOZ_MOUSE_SENS` | Mouse look sensitivity (default 12000) |
| `BOZ_LOOK_RADIUS` | Look stick radius on the touchpad (max 191) |
| `BOZ_LOOK_MODE=swipe` | Swipe-style mouse look |
| `BOZ_TRACE_STATUS=1` | Status line every 2 s |
| `BOZ_TRACE_CALLS=N` | Log the first N runtime calls |

## License

MIT, see `LICENSE` (upstream copyright retained).
