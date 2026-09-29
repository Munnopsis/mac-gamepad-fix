# MacGamepadFix 1.1.0

MacGamepadFix 1.1.0 extends the project beyond the original Unity-only compatibility path and adds validated support for Dead Island 2 on Apple Silicon macOS.

The release keeps the known-good Combat Master configuration working while introducing a second profile mode that does not inject a MacGamepadFix compatibility library into the game process.

## Highlights

* Validated Dead Island 2 support for Steam App ID `934700`.
* New `driver=none` profile mode.
* Dedicated `MacGamepadFix-DeadIsland2` launcher.
* UE4 macOS HID-controller backend configuration for Dead Island 2.
* Existing Combat Master Unity compatibility path remains functional.
* Patched SDL3 and bundled libusb Steam runtime retained.

## Validated games

### Combat Master

Combat Master continues to use the original MacGamepadFix game-process compatibility path:

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
Steam virtual GamePad-1
        |
        v
libMacGamepadFix.dylib
        +
libMacGamepadFixGCBlock.dylib
        |
        v
Combat Master
```

Validated profile:

```text
steam_app_id=2281730
left_y=invert
right_y=invert
gamecontroller=block
```

The Unity compatibility driver corrects the validated controller mapping and stick orientation while the GameController blocker prevents the competing Apple GameController path inside Combat Master.

### Dead Island 2

Dead Island 2 uses a different compatibility path.

The game is not injected with either MacGamepadFix game-process dynamic library.

The validated configuration uses:

```text
Steam App ID: 934700
driver=none
Steam Input: enabled per game
Slate.MacControllerPreferGCImpl=0
```

The resulting path is:

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

The Dead Island 2 launcher verifies that Steam is running through the MacGamepadFix Steam runtime and ensures that the UE4 macOS controller-backend setting is present.

Start Dead Island 2 with:

```bash
./MacGamepadFix --profile dead-island-2
```

Steam Input must be explicitly enabled for Dead Island 2 in Steam:

```text
Dead Island 2
-> Properties
-> Controller
-> Override for Dead Island 2
-> Enable Steam Input
```

## Driver modes

Profiles can now select the game-process compatibility mode.

### Unity driver

```text
driver=unity
```

This is the default for backward compatibility.

The Unity compatibility library is injected into the target game process.

Combat Master continues to use this mode.

### No game driver

```text
driver=none
```

No MacGamepadFix game-process compatibility library is injected.

Dead Island 2 uses this mode.

## Steam runtime

The bundled Steam runtime remains:

```text
steam/libSDL3.dylib
steam/libusb-1.0.0.dylib
```

Start Steam with:

```bash
./MacGamepadFix-Steam
```

Check its status with:

```bash
./MacGamepadFix-Steam --status
```

If Steam is already running without the compatibility runtime, restart it with:

```bash
./MacGamepadFix-Steam --restart
```

The patched SDL removes `DYLD_LIBRARY_PATH` after loading. This prevents the Steam SDL/libusb override from propagating into Steam-launched games.

## Included runtime files

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
  combat-master.conf
  dead-island-2.conf
  default-example.conf
```

## Validation

MacGamepadFix 1.1.0 was validated with both supported game profiles after the launcher architecture was extended.

Confirmed for Dead Island 2:

* controller input works in game;
* `Driver: none` is selected;
* no MacGamepadFix game dynamic library is loaded into the Dead Island 2 process;
* the Steam SDL/libusb compatibility runtime is not inherited by the game process.

Confirmed for Combat Master:

* controller input continues to work;
* `libMacGamepadFix.dylib` is loaded;
* `libMacGamepadFixGCBlock.dylib` is loaded;
* the Steam SDL/libusb compatibility runtime is not inherited by the game process.

## Licensing

The original MacGamepadFix code remains available under the PolyForm Noncommercial License 1.0.0.

Commercial use not permitted by that license requires a separate commercial license from the MacGamepadFix copyright holder.

SDL3, libusb and other third-party components remain under their respective licenses.

See:

```text
LICENSE
LICENSING.md
THIRD_PARTY_NOTICES.md
```

## Platform

MacGamepadFix 1.1.0 is currently built for Apple Silicon (`arm64`).

The local release binaries use ad-hoc signing and are not notarized.
