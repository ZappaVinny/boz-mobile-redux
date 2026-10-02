# Roadmap

BOZ Redux brings the original *Call of Duty: Black Ops Zombies* mobile game (Android, 2011–2016) to desktop and builds a modding ecosystem around it. You bring your own copy of the game; we provide the client, the launcher and the tools.

## Where we are

- The original game code runs on desktop Linux and Windows through a built-in ARM emulator. The full game is playable with mouse and keyboard, in a resizable or fullscreen window.
- Automated builds produce a Linux tarball and a Windows zip.
- No game files are included in this project, and none ever will be.

## How it fits together

| Layer | What it is | Examples |
| --- | --- | --- |
| **Client** | What every player needs to run the game well on a PC | Windows/Linux/macOS support, audio, performance, controls, launcher, mod loading |
| **Redux mod** | A base mod loaded automatically, built with the same tools anyone can use | FOV slider, better mouse sensitivity, extra settings |
| **Mods** | Anything the community makes | New textures, UI, weapons, gameplay changes, maps |

Gameplay changes always live in mods, never hard-coded into the client.

## Milestones

### 1. Cross-platform client
- One build system for Linux and Windows, with automated builds.
- Windows support (bundled Mesa: GPU through Direct3D 12, software fallback).
- Download a zip, run it, play.

### 2. Client essentials
- Audio and music.
- Smoother frame pacing and better performance.
- Crash fixes and crash logs.
- A settings file, rebindable controls, controller support, fullscreen toggle, higher internal resolution.

### 3. Launcher
- Built into the client.
- Set up your game files: choose your APK (it checks the version and warns about tampered copies), then get the data packs from Activision's server or import your own.
- Enable, disable and order mods; change settings; play.

### 4. Mod runtime
- **Asset mods:** drop replacement files into a mod folder, and they override the originals without touching your game files.
- **Code mods in Lua:** hook game functions, read and change game state, add settings to the launcher.
- **A shared symbol database:** named game functions and data, so mods don't break when internals are mapped differently.

### 5. Redux base mod
- FOV, uncapped mouse look, sensitivity curves, quality-of-life settings.
- The first real mod, and the reference example for mod authors.

### 6. Asset tools (bozkit)
- Open the game's `.group.bin` resource format, which holds every texture, model, UI screen and map.
- Textures to and from PNG; models to and from glTF; UI, audio and text editing.
- A Blender add-on for models and maps.

### 7. Maps
- Edit existing maps first.
- Then research what it takes to build brand-new ones: spawns, zombie pathing, barriers, rounds.

### 8. macOS
- A 64-bit version of the client, which macOS requires and which also enables a faster emulator backend.

## Mod format (planned)

A mod is a folder or `.zip`:

    mods/my-mod/
      mod.toml      name, version, author, game version, dependencies
      assets/       replacement files, using the game's own paths
      scripts/      main.lua

## Getting involved

- **Players:** testing on different hardware, especially Windows, helps the most right now.
- **Modders:** the mod format and Lua API will be documented here as they land.
- **Reverse engineers:** the symbol database and the `.group.bin` format are the big shared projects.

## Credits

Built on [cod-boz-port](https://github.com/Producdevity/cod-boz-port) by Producdevity, [Unicorn](https://github.com/unicorn-engine/unicorn), [SDL](https://github.com/libsdl-org/SDL) and [destin](https://github.com/Tatsh/destin).
