#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

SRC="$ROOT/src"
BUILD="$ROOT/build"
DIST="$ROOT/dist"

echo "== MacGamepadFix 1.1.0 =="

echo "[1/6] Checking Steam runtime..."

test -x "$ROOT/MacGamepadFix-Steam"
test -x "$ROOT/MacGamepadFix-DeadIsland2"
test -f "$ROOT/vendor/steam/libSDL3.dylib"
test -f "$ROOT/vendor/steam/libusb-1.0.0.dylib"

if otool -L "$ROOT/vendor/steam/libSDL3.dylib" |
    grep -q '/opt/homebrew'; then

    echo "ERROR: Steam SDL still contains a Homebrew dependency."
    exit 1
fi

codesign --verify \
"$ROOT/vendor/steam/libSDL3.dylib"

codesign --verify \
"$ROOT/vendor/steam/libusb-1.0.0.dylib"

echo "[2/6] Generating configurable Unity driver..."

python3 "$ROOT/tools/generate_driver.py"

echo "[3/6] Preparing directories..."

rm -rf "$BUILD" "$DIST"

mkdir -p \
"$BUILD" \
"$DIST/lib" \
"$DIST/steam" \
"$DIST/profiles"

echo "[4/6] Building Unity driver..."

clang -O2 \
-dynamiclib \
-fobjc-arc \
-fblocks \
"$SRC/driver/unity_input.c" \
"$SRC/driver/bootstrap.m" \
-framework Foundation \
-framework GameController \
-Wl,-install_name,@rpath/libMacGamepadFix.dylib \
-o "$BUILD/libMacGamepadFix.dylib"

codesign \
--force \
--sign - \
"$BUILD/libMacGamepadFix.dylib"

echo "[5/6] Building GameController blocker..."

clang -O2 \
-dynamiclib \
-fobjc-arc \
-fblocks \
"$SRC/driver/gc_block.m" \
-framework Foundation \
-framework GameController \
-Wl,-install_name,@rpath/libMacGamepadFixGCBlock.dylib \
-o "$BUILD/libMacGamepadFixGCBlock.dylib"

codesign \
--force \
--sign - \
"$BUILD/libMacGamepadFixGCBlock.dylib"

echo "[6/6] Building launcher and assembling runtime..."

clang -O2 \
-Wall \
-Wextra \
"$SRC/launcher/main.c" \
-o "$BUILD/MacGamepadFix"

codesign \
--force \
--sign - \
"$BUILD/MacGamepadFix"

cp "$BUILD/MacGamepadFix" \
"$DIST/MacGamepadFix"

cp "$ROOT/MacGamepadFix-Steam" \
"$DIST/MacGamepadFix-Steam"

cp "$ROOT/MacGamepadFix-DeadIsland2" \
"$DIST/MacGamepadFix-DeadIsland2"

chmod +x \
"$DIST/MacGamepadFix" \
"$DIST/MacGamepadFix-Steam" \
"$DIST/MacGamepadFix-DeadIsland2"

cp "$BUILD/libMacGamepadFix.dylib" \
"$DIST/lib/libMacGamepadFix.dylib"

cp "$BUILD/libMacGamepadFixGCBlock.dylib" \
"$DIST/lib/libMacGamepadFixGCBlock.dylib"

cp "$ROOT/vendor/steam/libSDL3.dylib" \
"$DIST/steam/libSDL3.dylib"

cp "$ROOT/vendor/steam/libusb-1.0.0.dylib" \
"$DIST/steam/libusb-1.0.0.dylib"

cp "$ROOT"/profiles/*.conf \
"$DIST/profiles/"

cp "$ROOT/VERSION" \
"$DIST/VERSION"

echo
echo "Build complete:"
echo "$DIST"
echo

find "$DIST" \
-maxdepth 3 \
-type f \
-print |
sort
