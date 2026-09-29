# MacGamepadFix

**macOS gamepad compatibility layer for affected games and Unity Input System configurations.**

Current release: **1.1.0**

MacGamepadFix provides game-specific compatibility paths for wired gamepads that are detected by macOS but are mapped incorrectly or exposed through an incompatible controller backend.

The project is built around the wired Xbox 360-compatible HID report layout and currently includes validated profiles for **Combat Master** and **Dead Island 2**.

MacGamepadFix is not a kernel driver and does not install a system-wide controller remapper. Depending on the profile, it either injects the Unity compatibility library into the game process or uses a non-injected Steam/UE4 compatibility path.

---

## Why MacGamepadFix exists

Some macOS games, particularly games using affected Unity Input System configurations, can detect a wired Xbox 360-compatible controller as a generic HID joystick instead of as a correctly mapped gamepad.

Typical symptoms include:

* inverted vertical stick axes;
* incorrect or incomplete gamepad mappings;
* broken or inconsistent D-pad behavior;
* conflicts between Unity HID input and Apple's GameController framework.

For Unity-based profiles, MacGamepadFix converts the affected HID device into a correctly defined Unity `Gamepad` layout before the game uses it. Dead Island 2 instead uses the validated Steam Input and UE4 HID-controller path without a MacGamepadFix game-process library.

Game-specific corrections, such as Y-axis inversion, are controlled through profiles or command-line options.

---

## How it works

MacGamepadFix supports multiple game-specific compatibility paths.

The runtime can include:

```text
MacGamepadFix
MacGamepadFix-Steam
MacGamepadFix-DeadIsland2

lib/
  libMacGamepadFix.dylib
  libMacGamepadFixGCBlock.dylib

steam/
  libSDL3.dylib
  libusb-1.0.0.dylib

profiles/
```

### `MacGamepadFix`

The main executable is the profile launcher.

It:

1. loads a game profile;

2. applies command-line overrides;

3. selects the requested game-process driver mode;

4. configures stick orientation and the optional GameController blocker when the Unity driver is used;

5. configures `DYLD_INSERT_LIBRARIES` only for injected profiles;

6. sets Steam application variables when required;

7. launches the configured game executable or game-specific compatibility launcher.

Profiles using:

```text
driver=unity
```

inject the Unity compatibility library into the target game process.

Profiles using:

```text
driver=none
```

do not inject a MacGamepadFix game library. The launcher explicitly removes inherited `DYLD_INSERT_LIBRARIES` before launching the configured target.

Dead Island 2 uses this non-injected mode.

### `MacGamepadFix-Steam`

This launcher starts or restarts Steam with the bundled patched SDL3 and libusb compatibility runtime.

The Steam-side runtime stabilizes the validated physical controller before Steam Input exposes it to games.

The patched SDL removes `DYLD_LIBRARY_PATH` after loading so the custom Steam runtime is not inherited by Steam-launched games.

### `MacGamepadFix-DeadIsland2`

This is a game-specific launcher used by the Dead Island 2 profile.

It verifies that Steam is running with the MacGamepadFix Steam runtime and ensures that the validated UE4 macOS controller-backend setting is present.

It does not inject either MacGamepadFix game-process dynamic library into Dead Island 2.

### `libMacGamepadFix.dylib`

This is the Unity/HID compatibility layer.

After the target process starts, it waits for the Unity runtime to become available and accesses Unity's IL2CPP runtime through `GameAssembly.dylib`.

It obtains the current joystick's device description and registers a custom Unity Input System `Gamepad` layout matching that device.

The current Xbox 360-compatible report mapping defines:

```text
D-pad                  Byte 2, bits 0-3
Start                   Byte 2, bit 4
Back / Select           Byte 2, bit 5
Left Stick Press        Byte 2, bit 6
Right Stick Press       Byte 2, bit 7

Left Shoulder / LB      Byte 3, bit 0
Right Shoulder / RB     Byte 3, bit 1
Xbox / Guide            Byte 3, bit 2

A / buttonSouth         Byte 3, bit 4
B / buttonEast          Byte 3, bit 5
X / buttonWest          Byte 3, bit 6
Y / buttonNorth         Byte 3, bit 7

Left Trigger / LT       Byte 4
Right Trigger / RT      Byte 5

Left Stick X/Y          Bytes 6-9
Right Stick X/Y         Bytes 10-13
```

Both analog sticks are represented as signed 16-bit X/Y controls and expose their normal Unity child controls:

```text
leftStick/x
leftStick/y
leftStick/left
leftStick/right
leftStick/up
leftStick/down

rightStick/x
rightStick/y
rightStick/left
rightStick/right
rightStick/up
rightStick/down
```

### Stick orientation

MacGamepadFix contains four generated gamepad layouts:

```text
left normal  / right normal
left invert  / right normal
left normal  / right invert
left invert  / right invert
```

The required layout is selected at runtime.

For an inverted Y axis, MacGamepadFix applies the correction consistently to the complete Unity stick representation rather than changing only the raw Y child control.

The working correction includes the complete `Stick` Vector2 path, the direct Y-axis path and the directional `up` / `down` controls.

This is important because games can access the same physical stick through different Unity Input System control paths.

### `libMacGamepadFixGCBlock.dylib`

This library is optional.

Some games receive the same physical controller through both the HID/Unity Input System path and Apple's GameController framework. That can interfere with the custom HID mapping.

When a profile uses:

```text
gamecontroller=block
```

MacGamepadFix additionally loads `libMacGamepadFixGCBlock.dylib`.

The blocker suppresses GameController device enumeration and controller connect/disconnect notifications inside the target process.

It should only be enabled for games that require it.

---

## Combat Master

A profile for Combat Master is included:

```text
profiles/combat-master.conf
```

Current settings:

```ini
executable=~/Library/Application Support/Steam/steamapps/common/Combat Master/CombatMaster.app/Contents/MacOS/Combat Master
steam_app_id=2281730

left_y=invert
right_y=invert

gamecontroller=block
```

For the validated Combat Master configuration, first start Steam through the bundled compatibility runtime:

```bash

./MacGamepadFix-Steam

```

If Steam is already running normally, restart it through MacGamepadFix:

```bash

./MacGamepadFix-Steam --restart

```

Then start Combat Master with:

```bash

./MacGamepadFix --profile combat-master

```

For diagnostic output:

```bash

./MacGamepadFix --profile combat-master --verbose

```

To inspect the configuration without launching the game:

```bash

./MacGamepadFix --profile combat-master --dry-run --verbose

```

---

## Dead Island 2

MacGamepadFix 1.1.0 includes a validated profile for Dead Island 2:

```text
profiles/dead-island-2.conf
```

The Dead Island 2 profile uses:

```text
steam_app_id=934700
driver=none
left_y=normal
right_y=normal
gamecontroller=auto
```

Unlike Combat Master, Dead Island 2 does not use the Unity compatibility library or the GameController blocker.

Steam must first be running through the bundled MacGamepadFix Steam runtime:

```bash
./MacGamepadFix-Steam --restart
```

Steam Input must also be explicitly enabled for Dead Island 2:

```text
Dead Island 2
-> Properties
-> Controller
-> Override for Dead Island 2
-> Enable Steam Input
```

Start the game with:

```bash
./MacGamepadFix --profile dead-island-2
```

The dedicated Dead Island 2 launcher checks that Steam is running with the MacGamepadFix runtime.

It also ensures that the following UE4 macOS controller setting is present in:

```text
~/Library/Preferences/DeadIsland/MacNoEditor/Engine.ini
```

Required setting:

```ini
[SystemSettings]
Slate.MacControllerPreferGCImpl=0
```

The validated Dead Island 2 process does not load:

```text
libMacGamepadFix.dylib
libMacGamepadFixGCBlock.dylib
```

The Steam SDL/libusb compatibility runtime is also not inherited by the Dead Island 2 process.

The validated input path is therefore:

```text
Physical Nacon controller
        |
        v
patched SDL3 + bundled libusb
        |
        v
Steam
        |
        v
Steam Input
        |
        v
Dead Island 2 / UE4 HID controller path
```

---

## Command-line interface

```text
MacGamepadFix --profile NAME [options]
MacGamepadFix --game EXECUTABLE [options]
```

Available options:

```text
--profile NAME
    Load profiles/NAME.conf.

--game EXECUTABLE
    Launch the specified game executable.
    Overrides the executable defined by a profile.

--steam-app-id ID
    Set SteamAppId and SteamGameId for the launched process.

--left-y normal|invert
    Select the left-stick Y orientation.

--right-y normal|invert
    Select the right-stick Y orientation.

--gamecontroller auto|block
    auto:
        Do not load the GameController blocker.

    block:
        Load libMacGamepadFixGCBlock.dylib in addition to the
        main compatibility library.

--dry-run
    Print the resolved configuration without launching the game.

--verbose
    Enable additional launcher and driver diagnostic output.

--list-profiles
    Show available profiles.

--version
    Print the MacGamepadFix version.

--help
    Show command-line help.

--
    Stop parsing MacGamepadFix options.
    All following arguments are passed directly to the game.
```

Command-line settings override profile settings.

Example:

```bash
./MacGamepadFix \
  --profile combat-master \
  --right-y normal
```

This loads the Combat Master profile but overrides the right-stick Y setting for that launch.

---

## Using MacGamepadFix with another game

A generic example profile is included:

```text
profiles/default-example.conf
```

A minimal profile looks like:

```ini
executable=/path/to/Game.app/Contents/MacOS/Game

driver=unity

left_y=normal
right_y=normal

gamecontroller=auto
```

The `driver` profile field selects the game-process compatibility mode:

```text
driver=unity
    Inject the Unity compatibility driver.
    This is the default when `driver` is omitted.

driver=none
    Do not inject a MacGamepadFix game library.
```

Start it with:

```bash
./MacGamepadFix --profile my-game
```

A profile must be stored as:

```text
profiles/my-game.conf
```

You can also launch a game without creating a profile:

```bash
./MacGamepadFix \
  --game "/path/to/Game.app/Contents/MacOS/Game" \
  --left-y normal \
  --right-y normal \
  --gamecontroller auto
```

---

## Choosing stick settings

Do not assume every game needs the Combat Master settings.

Start with:

```text
left_y=normal
right_y=normal
```

Only enable inversion when the corresponding vertical stick direction is wrong in that game.

For example:

```text
left_y=invert
right_y=normal
```

or:

```text
left_y=invert
right_y=invert
```

Button, D-pad, trigger and stick-button mappings are hardware mappings and are intentionally not game-specific profile options.

MacGamepadFix uses standard Unity/Xbox gamepad semantics:

```text
A = buttonSouth
B = buttonEast
X = buttonWest
Y = buttonNorth

LB = leftShoulder
RB = rightShoulder

LT = leftTrigger
RT = rightTrigger

L3 = leftStickPress
R3 = rightStickPress
```

---

## GameController mode

Use:

```text
gamecontroller=auto
```

by default.

Use:

```text
gamecontroller=block
```

only when the game's GameController input path interferes with the HID mapping.

Combat Master currently uses `block`.

Blocking GameController is process-local. It does not disable GameController support system-wide.

---

## Logs

The current driver writes diagnostic information to temporary files under `/tmp`.

Relevant files include:

```text
/tmp/cm-gamepad-phase1.log
/tmp/cm-gcdisable.log
```

For a clean diagnostic run:

```bash
rm -f \
/tmp/cm-gcdisable.log \
/tmp/cm-gamepad-phase1.log \
/tmp/cm-gamepad-phase2.log

./MacGamepadFix \
  --profile combat-master \
  --verbose
```

After exiting the game:

```bash
cat /tmp/cm-gamepad-phase1.log
cat /tmp/cm-gcdisable.log
```

---

## Building from source

Requirements:

* macOS;
* Xcode Command Line Tools or Xcode;
* Clang;
* Python 3;
* Make.

Build with:

```bash
make
```

The build process:

1. verifies the known-good gamepad layout reference;
2. generates all four stick-orientation layouts;
3. generates the configurable Unity driver;
4. builds `libMacGamepadFix.dylib`;
5. builds the optional GameController blocker;
6. builds the `MacGamepadFix` launcher;
7. ad-hoc signs the generated Mach-O binaries;
8. creates the runtime tree under `dist/`.

Build output:

```text
dist/
├── MacGamepadFix
├── MacGamepadFix-Steam
├── MacGamepadFix-DeadIsland2
├── VERSION
├── lib/
│   ├── libMacGamepadFix.dylib
│   └── libMacGamepadFixGCBlock.dylib
├── steam/
│   ├── libSDL3.dylib
│   └── libusb-1.0.0.dylib
└── profiles/
    ├── combat-master.conf
    ├── dead-island-2.conf
    └── default-example.conf
```

Clean generated files with:

```bash
make clean
```

Create a complete release package with:

```bash
make release
```

---

## Runtime environment

The launcher configures these internal environment variables for the compatibility library:

```text
MGF_LEFT_Y_INVERT
MGF_RIGHT_Y_INVERT
MGF_VERBOSE
```

For profiles using `driver=unity`, it also configures:

```text
DYLD_INSERT_LIBRARIES
```

and, when a Steam App ID is configured:

```text
SteamAppId
SteamGameId
```

Normally these variables should not be set manually. Use profiles and the MacGamepadFix command-line interface instead.

---

## Compatibility

MacGamepadFix 1.1.0 is specifically designed around the wired Xbox 360-compatible HID mapping used during development.

The included Combat Master and Dead Island 2 profiles are the currently validated game configurations.

Other games may require different stick-orientation and GameController settings.

MacGamepadFix currently does not provide:

* automatic controller identification across arbitrary gamepad families;
* automatic game discovery;
* automatic profile detection;
* a graphical interface;
* system-wide controller remapping;
* a kernel or DriverKit driver.

The current release does not install a system-wide controller remapper. Runtime compatibility changes are scoped to the relevant Steam or game process, with game-specific user configuration applied only where required.

---

## Important limitations

### Hardened Runtime

Profiles using `driver=unity` rely on dynamic-library injection through `DYLD_INSERT_LIBRARIES`.

Applications with security settings that reject injected libraries may not work with the `driver=unity` compatibility mode.

This limitation applies to the injection-based compatibility path and does not inherently apply to `driver=none` profiles.

### Anti-cheat software

Some games or anti-cheat systems prohibit or reject injected libraries regardless of what the library changes.

When `driver=unity` is used, MacGamepadFix only changes controller compatibility, but technically the compatibility code is still an injected dynamic library.

Do not use the `driver=unity` mode with software whose rules prohibit dynamic-library injection.

### Signing

The local 1.1.0 build uses ad-hoc code signing.

A public downloadable distribution should eventually use an appropriate Developer ID signature and Apple notarization.

### Architecture

The current build system compiles for the architecture of the Mac on which the release is built.

Universal binaries are not generated automatically in version 1.1.0.

---

## Project layout

```text
MacGamepadFix/
├── MacGamepadFix-Steam
├── MacGamepadFix-DeadIsland2
├── src/
│   ├── driver/
│   │   ├── bootstrap.m
│   │   ├── gc_block.m
│   │   └── unity_input_v12_reference.c
│   │
│   └── launcher/
│       └── main.c
│
├── tools/
│   ├── build.sh
│   ├── generate_driver.py
│   └── release.sh
│
├── profiles/
│   ├── combat-master.conf
│   ├── dead-island-2.conf
│   └── default-example.conf
│
├── vendor/steam/
├── third_party/
├── Makefile
├── README.md
├── CHANGELOG.md
├── RELEASE_NOTES_1.0.0.md
├── RELEASE_NOTES_1.1.0.md
└── VERSION
```

The source repository includes the Steam runtime libraries under `vendor/steam/`
and the third-party source archive, patch and license notices under `third_party/`.
These files are required by the build or release scripts. The active
`src/driver/unity_input_v12_reference.c` is an input to the driver generator.

The build generates `src/driver/unity_input.c`, `src/driver/generated_layouts.h`,
`build/` and `dist/`. Release packages are generated under `release/`. These
outputs and the optional local development snapshots in `legacy/` are ignored
by Git.

Keep personal notes and configuration in `private/` or `notes/`.
Environment files, local credentials, recordings and dependency
directories are also ignored. Any `.env.example`, `.env.sample` or
`.env.template` committed to the repository must contain placeholders only.

---

## Release 1.1.0

MacGamepadFix 1.1.0 adds Dead Island 2 as the second validated game configuration and introduces support for profiles that intentionally do not inject a game-process compatibility library.

The release includes:

```text
native MacGamepadFix launcher
Steam SDL/libusb compatibility runtime
configurable Unity HID compatibility library
optional GameController blocker
driver=unity and driver=none profile modes
Combat Master profile
Dead Island 2 profile
dedicated Dead Island 2 launcher
verbose diagnostics
dry-run support
release packaging
SHA-256 checksums
```

The two validated game-side compatibility modes are:

```text
Combat Master
    Unity driver + GameController blocker

Dead Island 2
    no MacGamepadFix game-process injection
    Steam Input + UE4 HID controller path
```

The Combat Master behavior validated for 1.0.0 remains intact.

See `RELEASE_NOTES_1.1.0.md` for the complete 1.1.0 release notes.

### Release 1.0.0

MacGamepadFix 1.0.0 was the first stable release and established the original Unity-based Combat Master compatibility path.

Its original release notes remain available in `RELEASE_NOTES_1.0.0.md`.

---

## Steam compatibility runtime

The validated Combat Master configuration requires two separate compatibility layers:

1. a Steam-side SDL/HIDAPI compatibility runtime;
2. the MacGamepadFix Unity Input System driver.

These layers solve different problems.

### Steam-side controller stabilization

The affected physical Nacon controller uses:

```text
Vendor ID:  0x146b
Product ID: 0x0603
```

During SDL HIDAPI re-enumeration, the controller can temporarily disappear from an enumeration pass even though the underlying libusb device is still alive.

Without the compatibility patch, Steam can repeatedly remove and rediscover the controller. This results in continuous controller connected/disconnected notifications.

MacGamepadFix therefore includes a patched SDL3 runtime which keeps the live controller registered when all of the following are true:

- the affected Nacon device is temporarily not seen during re-enumeration;
- an SDL joystick is already registered for it;
- the underlying libusb device is still active.

The Steam runtime consists of:

```text
steam/
├── libSDL3.dylib
└── libusb-1.0.0.dylib
```

The bundled SDL uses the bundled libusb library through:

```text
@loader_path/libusb-1.0.0.dylib
```

Homebrew is therefore not required at runtime.

### Starting Steam

For the validated configuration, Steam must first be started through:

```bash
./MacGamepadFix-Steam
```

Check whether Steam is already using the MacGamepadFix runtime with:

```bash
./MacGamepadFix-Steam --status
```

If Steam is already running without MacGamepadFix, restart it with:

```bash
./MacGamepadFix-Steam --restart
```

The Steam launcher enables:

```text
SDL_HIDAPI_LIBUSB=1
SDL_HIDAPI_LIBUSB_WHITELIST=0
SDL_JOYSTICK_HIDAPI=1
SDL_JOYSTICK_HIDAPI_XBOX=1
SDL_JOYSTICK_HIDAPI_XBOX_360=1
```

The patched SDL is selected for the Steam process using:

```text
DYLD_LIBRARY_PATH=<MacGamepadFix>/steam
```

Immediately after SDL is loaded, the patched SDL removes `DYLD_LIBRARY_PATH` from the Steam process environment.

This is intentional. It prevents Combat Master and other applications launched by Steam from unintentionally inheriting the SDL override.

### Starting Combat Master

After Steam is running with the MacGamepadFix Steam runtime, launch Combat Master with:

```bash
./MacGamepadFix --profile combat-master
```

For additional diagnostic information:

```bash
./MacGamepadFix --profile combat-master --verbose
```

To inspect the resolved configuration without starting the game:

```bash
./MacGamepadFix --profile combat-master --dry-run --verbose
```

### Complete input path

The validated Combat Master configuration works as follows:

```text
Physical Nacon controller
        │
        ▼
patched SDL3 + bundled libusb
        │
        ▼
stable controller in Steam Input
        │
        ▼
Steam virtual GamePad-1
        │
        ▼
MacGamepadFix Unity compatibility driver
        │
        ▼
correctly mapped controller in Combat Master
```

The Steam-side SDL compatibility runtime and the Unity-side MacGamepadFix driver are both required for the currently validated Combat Master configuration.

The optional GameController blocker used by the Combat Master profile operates only inside the game process. It prevents Apple's GameController input path from competing with the custom Unity HID mapping.

---

## Licensing

The original MacGamepadFix code is source-available under the PolyForm Noncommercial License 1.0.0.

SPDX identifier: `PolyForm-Noncommercial-1.0.0`

Uses permitted by that license are available without a commercial MacGamepadFix license.

Commercial use of the original MacGamepadFix code that is not permitted by the PolyForm Noncommercial License requires a separate commercial license from the MacGamepadFix copyright holder.

See `LICENSE` for the complete PolyForm license terms and `LICENSING.md` for the MacGamepadFix licensing overview.

This licensing model is source-available rather than OSI Open Source because commercial use is restricted.

Third-party components are not relicensed by MacGamepadFix. SDL3 and libusb remain subject to their respective licenses. See `THIRD_PARTY_NOTICES.md` and the files under `third_party/licenses/`.
