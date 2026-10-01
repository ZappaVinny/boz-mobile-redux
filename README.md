# Classic Black Ops Zombies Mobile Redux

A desktop port and modding base for the original Android *Call of Duty: Black Ops Zombies*. It runs the game's own ARM code on an x86 Linux PC through a built-in ARM emulator, with keyboard and mouse controls and a resizable or fullscreen window.

This repo contains no game files. You need your own copy of the Android 1.0.11 APK.

See [ROADMAP.md](ROADMAP.md) for where the project is headed.

## Creation Information
This project is being done by a human guided Generative AI (LLM) system, with minimal human verification besides functionality testing. Ideally the mod tools become human workable without AI. 

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
make                          # builds Unicorn, SDL2, the loader and the APK extractor
make test
scripts/setup-game.sh         # extracts the APK into assets/ and links the data packs
scripts/run-desktop.sh
```

`setup-game.sh` reads `original/com.activision.boz.apk` and packs from `original/obb/` by default, and downloads any missing pack from Activision's CDN. Pass other paths as arguments: `setup-game.sh <apk> <packs-dir> <game-dir>`.

Game data goes in `assets/` and your saves in `saves/`, both at the repo root and both gitignored.

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

Mouse speed: `BOZ_MOUSE_SENS` (default 12000).

## Credits

The runtime is a fork of [cod-boz-port](https://github.com/Producdevity/cod-boz-port) by Producdevity (MIT). It uses [Unicorn](https://github.com/unicorn-engine/unicorn) and [SDL2](https://github.com/libsdl-org/SDL). Asset tools come from [destin](https://github.com/Tatsh/destin).

## License

MIT, see [LICENSE](LICENSE). The forked runtime keeps its original MIT notice in [runtime/LICENSE](runtime/LICENSE). Game files belong to Activision and are not covered by this license or included in this repo.

## Contribution
Anyone is allowed to contribute, just make a PR. The project is under MIT licences, so have fun!