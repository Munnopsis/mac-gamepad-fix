# MacGamepadFix 1.0.0

MacGamepadFix 1.0.0 is the first stable release of the macOS gamepad compatibility layer developed from the validated Xbox 360 / Unity Input System mapping.

The release combines a configurable Unity compatibility driver with a Steam-side SDL/HIDAPI compatibility runtime for the validated Combat Master controller configuration.

## Highlights

- Native MacGamepadFix command-line launcher.
- Configurable Unity HID compatibility driver.
- Independent left- and right-stick Y-axis orientation.
- Optional Apple GameController blocker.
- Patched SDL3 runtime for stable Steam controller handling.
- Bundled libusb 1.0.30 runtime; Homebrew is not required at runtime.
- Dedicated `MacGamepadFix-Steam` launcher.
- Combat Master included as the first validated game profile.

## Controller compatibility path

The validated Combat Master configuration uses two separate compatibility layers.

The physical Nacon controller has:

```text
Vendor ID:  0x146b
Product ID: 0x0603
```

A targeted SDL HIDAPI patch prevents this controller from being incorrectly removed when it temporarily disappears during re-enumeration while its libusb device is still alive.

Steam then exposes the stable virtual controller used by the Unity compatibility layer:

```text
HID::Microsoft GamePad-1
```

The resulting input path is:

```text
Physical Nacon controller
        |
        v
patched SDL3 + bundled libusb
        |
        v
stable Steam Input controller
        |
        v
Steam virtual GamePad-1
        |
        v
MacGamepadFix Unity driver
        |
        v
Combat Master
```

## Steam runtime

Steam must be started through the bundled compatibility launcher:

```bash
./MacGamepadFix-Steam
```

Check whether Steam is already using the MacGamepadFix runtime:

```bash
./MacGamepadFix-Steam --status
```

If Steam is already running normally, restart it through MacGamepadFix:

```bash
./MacGamepadFix-Steam --restart
```

The bundled Steam runtime contains:

```text
steam/libSDL3.dylib
steam/libusb-1.0.0.dylib
```

The SDL library uses the bundled libusb through `@loader_path`, so a Homebrew libusb installation is not required at runtime.

The patched SDL removes `DYLD_LIBRARY_PATH` after loading. This prevents the Steam SDL override from propagating into Combat Master or other Steam-launched applications.

## Combat Master

After Steam is running through `MacGamepadFix-Steam`, start Combat Master with:

```bash
./MacGamepadFix --profile combat-master
```

Diagnostic launch:

```bash
./MacGamepadFix --profile combat-master --verbose
```

Configuration check without launching:

```bash
./MacGamepadFix --profile combat-master --dry-run --verbose
```

The validated Combat Master profile uses:

```text
left_y=invert
right_y=invert
gamecontroller=block
steam_app_id=2281730
```

## Included runtime files

```text
MacGamepadFix
MacGamepadFix-Steam

lib/
  libMacGamepadFix.dylib
  libMacGamepadFixGCBlock.dylib

steam/
  libSDL3.dylib
  libusb-1.0.0.dylib

profiles/
  combat-master.conf
  default-example.conf
```

## Included documentation and third-party materials

```text
README.md
CHANGELOG.md
RELEASE_NOTES_1.0.0.md
THIRD_PARTY_NOTICES.md
LICENSE
LICENSING.md
VERSION
BUILD_INFO.txt
SHA256SUMS

third_party/licenses/
  SDL-zlib.txt
  libusb-LGPL-2.1.txt

third_party/sdl/
  BASE_COMMIT
  sdl-steam-nacon.patch

third_party/libusb/
  libusb-1.0.30.tar.bz2
```

## Licensing

The original MacGamepadFix code is made available under the PolyForm Noncommercial License 1.0.0 (`PolyForm-Noncommercial-1.0.0`).

Uses permitted by that license do not require a separate commercial MacGamepadFix license.

Commercial use of the original MacGamepadFix code that is not permitted by the PolyForm Noncommercial License requires a separate commercial license from the MacGamepadFix copyright holder.

See `LICENSE` for the complete license terms and `LICENSING.md` for the licensing overview.

MacGamepadFix uses a source-available licensing model rather than an OSI Open Source license because commercial use is restricted.

SDL3, libusb and other third-party materials remain subject to their own licenses.

## Third-party components

The bundled SDL3 build is based on commit:

```text
2d7f300783215f6ae3361a43a3c71f02d813c621
```

The SDL modifications used by MacGamepadFix are included as `third_party/sdl/sdl-steam-nacon.patch`.

The bundled libusb runtime corresponds to libusb 1.0.30. The verified source archive is included in the release together with the applicable license texts and `THIRD_PARTY_NOTICES.md`.

## Validation

The final macOS arm64 ZIP was tested after extraction rather than from the development tree.

The validation confirmed:

- Steam loads the bundled SDL3 and libusb libraries from the extracted release.
- No older Steam SDL override is loaded.
- The controller remains stable in Steam without repeated disconnect/reconnect events.
- Combat Master loads both MacGamepadFix game-process libraries from the extracted release.
- The Steam SDL override is not loaded into Combat Master.
- Unity registers and matches `HID::Microsoft GamePad-1` without an exception.
- The GameController blocker is active for the Combat Master profile.
- Internal release checksums and the ZIP checksum validate successfully.

## Notes

MacGamepadFix is a process-local compatibility layer, not a system-wide macOS driver.

The game-side launcher uses `DYLD_INSERT_LIBRARIES` to load the compatibility code into the target process. Applications that prohibit dynamic-library injection may therefore be incompatible.

The Steam-side SDL override and the Unity-side compatibility driver solve different problems and are both required for the currently validated Combat Master configuration.

MacGamepadFix 1.0.0 is currently built for Apple Silicon (`arm64`).

The local release binaries are ad-hoc signed and are not notarized.
