# Changelog

All notable changes to MacGamepadFix are documented in this file.

## 1.1.0 — 2026-08-31

Second stable release.

### Added

* Dead Island 2 support for Steam App ID `934700`.

* New `driver=none` profile mode for games that do not require the Unity compatibility library.

* Dedicated `MacGamepadFix-DeadIsland2` launcher.

* Dead Island 2 UE4 macOS controller-backend configuration using:

  * `Slate.MacControllerPreferGCImpl=0`

* Support for profile executables located relative to the MacGamepadFix runtime directory.

### Changed

* The launcher now distinguishes between injected Unity-driver profiles and non-injected compatibility profiles.

* `--dry-run` reports `Driver: none` when no game-process compatibility library is requested.

* Build output now includes the Dead Island 2 launcher and profile.

### Validated

* Dead Island 2 works with the patched Steam SDL/libusb runtime and per-game Steam Input enabled.

* Dead Island 2 runs without `libMacGamepadFix.dylib` or `libMacGamepadFixGCBlock.dylib` injected into its process.

* The Steam SDL/libusb runtime does not propagate into the Dead Island 2 process.

* Combat Master continues to load the Unity compatibility driver and GameController blocker correctly.

* Combat Master controller behavior remains unchanged from 1.0.0.

### Compatibility modes

MacGamepadFix now has two validated game-side modes:

* Unity injection mode, used by Combat Master.

* Non-injected Steam/UE4 mode, used by Dead Island 2.

## 1.0.0 — 2026-08-31

First stable release.

### Added

* Native `MacGamepadFix` launcher.
* Configurable Unity HID gamepad compatibility library.
* Optional Apple GameController compatibility blocker.
* Game profiles stored as `.conf` files.
* Combat Master profile for Steam App ID `2281730`.
* Independent left-stick and right-stick Y-axis configuration.
* Four generated stick-orientation layouts:

  * normal / normal
  * invert / normal
  * normal / invert
  * invert / invert
* Correct Xbox-style semantic mapping for:

  * D-pad
  * A/B/X/Y
  * LB/RB
  * LT/RT
  * L3/R3
  * Start/Back
  * Guide/Xbox button
  * both analog sticks
* Explicit Unity child controls for stick directions.
* Vector2-level Y-axis correction for affected Unity input paths.
* `--profile`
* `--game`
* `--steam-app-id`
* `--left-y`
* `--right-y`
* `--gamecontroller`
* `--dry-run`
* `--verbose`
* `--list-profiles`
* `--version`
* `--help`
* Pass-through game arguments after `--`.
* Automated source generation.
* Automated local build and ad-hoc code signing.
* Release packaging and SHA-256 checksum generation.

### Validated

* Combat Master launches successfully through MacGamepadFix.
* Controller is registered as a Unity `Gamepad`.
* Left-stick Y direction is corrected.
* Right-stick Y direction is corrected.
* The known-good final development mapping is retained as the release reference.

### Known limitations

* Version 1.0.0 targets the Xbox 360-compatible wired HID mapping used during development.
* Other gamepad families are not automatically detected or configured.
* Dynamic-library injection can be rejected by Hardened Runtime or other application security mechanisms.
* Injected libraries may be incompatible with anti-cheat systems.
* The local release uses ad-hoc code signing.
* The build targets the architecture of the build machine rather than producing a Universal Binary.
* No graphical interface is included.

### Steam compatibility runtime

- Added bundled patched SDL3 runtime for Steam.
- Added bundled libusb runtime.
- Added `MacGamepadFix-Steam` launcher.
- Added detection of Steam already running without the compatibility runtime.
- Added `--status` and `--restart` Steam-launcher options.
- Added targeted handling for Nacon `146b:0603` HIDAPI re-enumeration.
- Prevented the Steam SDL override from propagating to Steam-launched games.
- Removed the runtime dependency on Homebrew's libusb installation.

### Project licensing

- Added the PolyForm Noncommercial License 1.0.0 for original MacGamepadFix code.
- Added `LICENSING.md` describing the noncommercial and separate commercial licensing paths.
- Commercial use not permitted by the PolyForm Noncommercial License requires a separate commercial license from the MacGamepadFix copyright holder.
- Third-party components such as SDL3 and libusb remain under their respective licenses.

