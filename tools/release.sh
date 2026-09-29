#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$(tr -d '[:space:]' < "$ROOT/VERSION")"
RELEASE_NOTES="$ROOT/RELEASE_NOTES_${VERSION}.md"

NAME="MacGamepadFix"
ARCH="$(uname -m)"

RELEASE_ROOT="$ROOT/release"
PACKAGE_NAME="${NAME}-${VERSION}"
PACKAGE="$RELEASE_ROOT/$PACKAGE_NAME"

ZIP_NAME="${PACKAGE_NAME}-macos-${ARCH}.zip"
ZIP="$RELEASE_ROOT/$ZIP_NAME"

LIBUSB_SOURCE_SHA256="fea36f34f9156400209595e300840767ab1a385ede1dc7ee893015aea9c6dbaf"

echo "== MacGamepadFix Release $VERSION =="

echo
echo "[1/9] Clean build..."

cd "$ROOT"

make clean
make

echo
echo "[2/9] Verifying runtime..."

test -x "$ROOT/dist/MacGamepadFix"
test -x "$ROOT/dist/MacGamepadFix-Steam"
test -x "$ROOT/dist/MacGamepadFix-DeadIsland2"

test -f "$ROOT/dist/lib/libMacGamepadFix.dylib"
test -f "$ROOT/dist/lib/libMacGamepadFixGCBlock.dylib"

test -f "$ROOT/dist/steam/libSDL3.dylib"
test -f "$ROOT/dist/steam/libusb-1.0.0.dylib"

codesign --verify \
"$ROOT/dist/MacGamepadFix"

codesign --verify \
"$ROOT/dist/lib/libMacGamepadFix.dylib"

codesign --verify \
"$ROOT/dist/lib/libMacGamepadFixGCBlock.dylib"

codesign --verify \
"$ROOT/dist/steam/libSDL3.dylib"

codesign --verify \
"$ROOT/dist/steam/libusb-1.0.0.dylib"

if otool -L "$ROOT/dist/steam/libSDL3.dylib" |
    grep -q '/opt/homebrew'; then

    echo "ERROR: Release SDL contains Homebrew path."
    exit 1
fi

if ! otool -L "$ROOT/dist/steam/libSDL3.dylib" |
    grep -q '@loader_path/libusb-1.0.0.dylib'; then

    echo "ERROR: Release SDL does not use bundled libusb."
    exit 1
fi

echo
echo "[3/9] Verifying third-party materials..."

test -f "$RELEASE_NOTES"
test -f "$ROOT/THIRD_PARTY_NOTICES.md"
test -f "$ROOT/LICENSE"
test -f "$ROOT/LICENSING.md"

test -f \
"$ROOT/third_party/licenses/SDL-zlib.txt"

test -f \
"$ROOT/third_party/licenses/libusb-LGPL-2.1.txt"

test -f \
"$ROOT/third_party/sdl/BASE_COMMIT"

test -f \
"$ROOT/third_party/sdl/sdl-steam-nacon.patch"

test -f \
"$ROOT/third_party/libusb/libusb-1.0.30.tar.bz2"

SDL_BASE_COMMIT="$(
    tr -d '[:space:]' \
    < "$ROOT/third_party/sdl/BASE_COMMIT"
)"

if [ "$SDL_BASE_COMMIT" != \
"2d7f300783215f6ae3361a43a3c71f02d813c621" ]; then

    echo "ERROR: Unexpected SDL base commit:"
    echo "$SDL_BASE_COMMIT"
    exit 1
fi

ACTUAL_LIBUSB_SOURCE_SHA256="$(
    shasum -a 256 \
    "$ROOT/third_party/libusb/libusb-1.0.30.tar.bz2" |
    awk '{print $1}'
)"

if [ "$ACTUAL_LIBUSB_SOURCE_SHA256" != \
"$LIBUSB_SOURCE_SHA256" ]; then

    echo "ERROR: libusb source checksum mismatch."
    echo "Expected: $LIBUSB_SOURCE_SHA256"
    echo "Actual:   $ACTUAL_LIBUSB_SOURCE_SHA256"
    exit 1
fi

echo "Third-party materials verified."

echo
echo "[4/9] Preparing package..."

rm -rf "$RELEASE_ROOT"

mkdir -p \
"$PACKAGE/lib" \
"$PACKAGE/steam" \
"$PACKAGE/profiles" \
"$PACKAGE/third_party/licenses" \
"$PACKAGE/third_party/sdl" \
"$PACKAGE/third_party/libusb"

echo
echo "[5/9] Copying runtime and documentation..."

cp "$ROOT/dist/MacGamepadFix" \
"$PACKAGE/MacGamepadFix"

cp "$ROOT/dist/MacGamepadFix-Steam" \
"$PACKAGE/MacGamepadFix-Steam"

cp "$ROOT/dist/MacGamepadFix-DeadIsland2" \
"$PACKAGE/MacGamepadFix-DeadIsland2"

cp "$ROOT/dist/lib/libMacGamepadFix.dylib" \
"$PACKAGE/lib/libMacGamepadFix.dylib"

cp "$ROOT/dist/lib/libMacGamepadFixGCBlock.dylib" \
"$PACKAGE/lib/libMacGamepadFixGCBlock.dylib"

cp "$ROOT/dist/steam/libSDL3.dylib" \
"$PACKAGE/steam/libSDL3.dylib"

cp "$ROOT/dist/steam/libusb-1.0.0.dylib" \
"$PACKAGE/steam/libusb-1.0.0.dylib"

cp "$ROOT"/profiles/*.conf \
"$PACKAGE/profiles/"

cp "$ROOT/README.md" \
"$PACKAGE/README.md"

cp "$ROOT/CHANGELOG.md" \
"$PACKAGE/CHANGELOG.md"

cp "$RELEASE_NOTES" \
"$PACKAGE/RELEASE_NOTES_${VERSION}.md"

cp "$ROOT/VERSION" \
"$PACKAGE/VERSION"

cp "$ROOT/THIRD_PARTY_NOTICES.md" \
"$PACKAGE/THIRD_PARTY_NOTICES.md"

cp "$ROOT/LICENSE" \
"$PACKAGE/LICENSE"

cp "$ROOT/LICENSING.md" \
"$PACKAGE/LICENSING.md"

echo
echo "[6/9] Copying third-party materials..."

cp "$ROOT/third_party/licenses/SDL-zlib.txt" \
"$PACKAGE/third_party/licenses/SDL-zlib.txt"

cp "$ROOT/third_party/licenses/libusb-LGPL-2.1.txt" \
"$PACKAGE/third_party/licenses/libusb-LGPL-2.1.txt"

cp "$ROOT/third_party/sdl/BASE_COMMIT" \
"$PACKAGE/third_party/sdl/BASE_COMMIT"

cp "$ROOT/third_party/sdl/sdl-steam-nacon.patch" \
"$PACKAGE/third_party/sdl/sdl-steam-nacon.patch"

cp "$ROOT/third_party/libusb/libusb-1.0.30.tar.bz2" \
"$PACKAGE/third_party/libusb/libusb-1.0.30.tar.bz2"

echo
echo "[7/9] Writing build information..."

{
    echo "MacGamepadFix $VERSION"
    echo
    echo "Release date: $(date '+%Y-%m-%d')"
    echo "Architecture: $ARCH"
    echo "macOS build host: $(sw_vers -productVersion)"
    echo
    echo "SDL base commit:"
    cat "$ROOT/third_party/sdl/BASE_COMMIT"
    echo
    echo "libusb source:"
    echo "libusb 1.0.30"
    echo "SHA-256: $LIBUSB_SOURCE_SHA256"
    echo
    echo "Compiler:"
    clang --version | head -1
    echo
    echo "Signing:"
    echo "Ad-hoc local signatures"
} > "$PACKAGE/BUILD_INFO.txt"

echo
echo "[8/9] Generating checksums..."

(
    cd "$PACKAGE"

    FILES=(
        "MacGamepadFix"
        "MacGamepadFix-Steam"
        "MacGamepadFix-DeadIsland2"
        "lib/libMacGamepadFix.dylib"
        "lib/libMacGamepadFixGCBlock.dylib"
        "steam/libSDL3.dylib"
        "steam/libusb-1.0.0.dylib"
        "profiles/combat-master.conf"
        "profiles/dead-island-2.conf"
        "profiles/default-example.conf"
        "README.md"
        "CHANGELOG.md"
        "RELEASE_NOTES_${VERSION}.md"
        "THIRD_PARTY_NOTICES.md"
        "LICENSE"
        "LICENSING.md"
        "VERSION"
        "BUILD_INFO.txt"
        "third_party/licenses/SDL-zlib.txt"
        "third_party/licenses/libusb-LGPL-2.1.txt"
        "third_party/sdl/BASE_COMMIT"
        "third_party/sdl/sdl-steam-nacon.patch"
        "third_party/libusb/libusb-1.0.30.tar.bz2"
    )
    shasum -a 256 \
    "${FILES[@]}" \
    > SHA256SUMS
)

echo
echo "[9/9] Creating ZIP and checksum..."

cd "$RELEASE_ROOT"

ditto \
-c \
-k \
--sequesterRsrc \
--keepParent \
"$PACKAGE_NAME" \
"$ZIP_NAME"

shasum -a 256 \
"$ZIP_NAME" \
> "$ZIP_NAME.sha256"

echo
echo "Release complete."
echo
echo "Package:"
echo "  $PACKAGE"
echo
echo "ZIP:"
echo "  $ZIP"
echo
echo "ZIP checksum:"
echo "  $ZIP.sha256"
echo
echo "Contents:"

find "$PACKAGE" \
-maxdepth 4 \
-type f |
sort
