#!/usr/bin/env bash
# Starts the game. Saves go to ./saves; set BOZ_WINDOWED=1 to start in a window (F11 toggles).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
[ -f "$here/assets/boz.s3e.unpacked" ] || { echo "Run ./setup.sh first." >&2; exit 1; }
if [ -z "${SDL_VIDEODRIVER:-}" ]; then
  if [ -n "${WAYLAND_DISPLAY:-}" ] && ldconfig -p | grep 'libxkbcommon.so.0 (libc6)' >/dev/null; then
    export SDL_VIDEODRIVER=wayland
  else
    export SDL_VIDEODRIVER=x11
  fi
fi
export XDG_CACHE_HOME="${XDG_CACHE_HOME:-$HOME/.cache}"
export HOME="${BOZ_SAVES:-$here/saves}"
mkdir -p "$HOME"
cd "$here"
exec "$here/codboz_s3e_loader" --root "$here" --display-size "${BOZ_DISPLAY:-1280x720}" "$@" \
  --run "$here/assets/boz.s3e.unpacked"
