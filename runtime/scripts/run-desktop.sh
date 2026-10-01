#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
game="${1:-$(cd "$here/.." && pwd)}"
shift || true
export LD_LIBRARY_PATH="$here/build/x86/sdl2-install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [ -z "${SDL_VIDEODRIVER:-}" ]; then
  if [ -n "${WAYLAND_DISPLAY:-}" ] && ldconfig -p | grep 'libxkbcommon.so.0 (libc6)' >/dev/null; then
    export SDL_VIDEODRIVER=wayland
  else
    export SDL_VIDEODRIVER=x11
  fi
fi
export BOZ_WINDOWED="${BOZ_WINDOWED-1}"
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$HOME/.cache}"
export HOME="${BOZ_SAVES:-$game/saves}"
mkdir -p "$HOME"
cd "$game"
exec "$here/build/x86/codboz_s3e_loader" --root "$game" --display-size "${BOZ_DISPLAY:-1280x720}" "$@" --run "$game/assets/boz.s3e.unpacked"
