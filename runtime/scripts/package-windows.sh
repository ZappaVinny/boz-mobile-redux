#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
build="$here/build/windows-x86"
stage="$build/package/BOZ-Redux"
rm -rf "$build/package"
mkdir -p "$stage"
cp "$build/bin/codboz_s3e_loader.exe" "$build/bin/codboz_apk_extract.exe" "$build/bin/SDL2.dll" "$stage/"
cp "$here/packaging/windows/setup.bat" "$here/packaging/windows/run.bat" "$stage/"
"$here/scripts/fetch-mesa-windows.sh"
cp "$build/mesa/"*.dll "$stage/"
cp "$here/packaging/THIRD-PARTY.txt" "$stage/"
for f in "$here/packaging/windows/"*.bat "$here/packaging/THIRD-PARTY.txt"; do sed -i 's/$/\r/' "$stage/$(basename "$f")"; done
(cd "$build/package" && rm -f BOZ-Redux-windows-x86.zip && zip -qr BOZ-Redux-windows-x86.zip BOZ-Redux)
echo "$build/package/BOZ-Redux-windows-x86.zip"
