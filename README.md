# Classic Black Ops Zombies Mobile Redux

A desktop port and modding base for the original Android *Call of Duty: Black Ops Zombies*. It runs the game's own ARM code on x86 Linux and Windows PCs through a built-in ARM emulator, with keyboard and mouse controls and a resizable or fullscreen window.

This repo contains no game files. You need your own copy of the Android 1.0.11 APK.

See [ROADMAP.md](ROADMAP.md) for where the project is headed.

## Creation Information
This project is being done by a human guided Generative AI (LLM) system, with minimal human verification besides functionality testing. Ideally the mod tools become human workable without AI. 

## Downloads

Builds for Linux (`BOZ-Redux-linux-x86.tar.gz`) and Windows (`BOZ-Redux-windows-x86.zip`) come from GitHub Actions: see the latest run's artifacts, or the Releases page for tagged versions. Put your APK next to `setup.sh` / `setup.bat`, run it once, then start the game with `run.sh` / `run.bat`.

## Requirements

Arch Linux with the multilib repo (For now in Proof of Concept):

```bash
sudo pacman -S --needed base-devel cmake lib32-glibc lib32-gcc-libs lib32-mesa lib32-libglvnd \
  lib32-libxkbcommon lib32-libdecor lib32-wayland lib32-libx11 lib32-alsa-lib
```

## Build and run (For now in Proof of Concept)

```bash
git submodule update --init --recursive
cd runtime
cmake --preset linux-x86                  # configure (32-bit)
cmake --build --preset linux-x86-tests     # builds Unicorn, SDL2, the loader, extractor and tests
ctest --preset linux-x86
scripts/setup-game.sh         # extracts the APK into assets/ and links the data packs
scripts/run-desktop.sh
```

`setup-game.sh` reads `original/com.activision.boz.apk` and packs from `original/obb/` by default, and downloads any missing pack from Activision's CDN. Pass other paths as arguments: `setup-game.sh <apk> <packs-dir> <game-dir>`.

Game data goes in `assets/` and your saves in `saves/`, both at the repo root and both gitignored.

### Windows build (cross-compiled from Linux)

Needs `mingw-w64-gcc` (the POSIX-threads variant), `zip` and `bsdtar`:

```bash
cd runtime
cmake --preset windows-x86
cmake --build --preset windows-x86
scripts/package-windows.sh    # downloads Mesa for Windows and writes build/windows-x86/package/BOZ-Redux-windows-x86.zip
```

The Windows client renders through a bundled Mesa (`opengl32.dll`). At startup it tests Mesa's GPU driver (Direct3D 12) and falls back to its software renderer when that can't open a window. Set `GALLIUM_DRIVER` to force one.

## Controls (For now in Proof of Concept, so are not functional)

| Input | Menus | Game (press Tab) |
| --- | --- | --- |
| Mouse | Pointer | Look |
| Left / right click | Tap / - | Shoot / aim |
| WASD | | Move |
| R / E / V | | Reload / action / melee |
| G / Q | | Grenade / tactical |
| C or Space | | Crouch |
| X / 1 | | Alt fire / switch weapon |
| Esc | | Pause |
| Tab | Switch to game mode | Switch to menus |
| F11 or Alt+Enter | Toggle fullscreen | Toggle fullscreen |

Settings live in `client.ini` next to the game data (the repo root when running from source), written with comments on first run: fullscreen, vsync, frame rate limit, scaling, mouse sensitivity and look mode. A matching `BOZ_*` environment variable overrides a setting for one run. Keyboard and mouse controls can be rebound in the `[keys]` section; controllers are mapped automatically.

## Credits

The runtime is a fork of [cod-boz-port](https://github.com/Producdevity/cod-boz-port) by Producdevity (MIT). It uses [Unicorn](https://github.com/unicorn-engine/unicorn) and [SDL2](https://github.com/libsdl-org/SDL). Asset tools come from [destin](https://github.com/Tatsh/destin).

## License

MIT, see [LICENSE](LICENSE). Release binaries include the Unicorn engine (GPLv2), so the loader binary as distributed is under the GPLv2; see `runtime/packaging/THIRD-PARTY.txt`. The forked runtime keeps its original MIT notice in [runtime/LICENSE](runtime/LICENSE). Game files belong to Activision and are not covered by this license or included in this repo.

## Contribution
Anyone is allowed to contribute, just make a PR. The project is under MIT licences, so have fun!