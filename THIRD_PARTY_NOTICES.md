# Third-Party Notices

MacGamepadFix 1.1.0 includes third-party components used by the optional Steam compatibility runtime.

## Simple DirectMedia Layer (SDL)

Component:
SDL3

Upstream project:
https://github.com/libsdl-org/SDL

Base commit:
2d7f300783215f6ae3361a43a3c71f02d813c621

License:
zlib License

License text:
third_party/licenses/SDL-zlib.txt

MacGamepadFix distributes a modified SDL build.

Debug symbol entries containing local build paths have been removed from the
bundled SDL library. The library has an ad-hoc code signature.

The modifications used by MacGamepadFix are recorded in:

third_party/sdl/sdl-steam-nacon.patch

The patch contains:

- targeted handling for the Nacon USB controller with vendor ID 0x146b and product ID 0x0603 during HIDAPI re-enumeration;
- a macOS constructor which removes DYLD_LIBRARY_PATH after the custom SDL library has been loaded by Steam.

The upstream source is not claimed to have been written by the MacGamepadFix project.

## libusb

Component:
libusb 1.0.30

Upstream project:
https://github.com/libusb/libusb

License:
GNU Lesser General Public License version 2.1 or later

License text:
third_party/licenses/libusb-LGPL-2.1.txt

Source archive:
third_party/libusb/libusb-1.0.30.tar.bz2

Source archive SHA-256:
fea36f34f9156400209595e300840767ab1a385ede1dc7ee893015aea9c6dbaf

The bundled macOS libusb dynamic library is used by the bundled SDL3 Steam compatibility runtime.

MacGamepadFix does not claim authorship of libusb.

## MacGamepadFix modifications

MacGamepadFix-specific source code and modifications are separate from the upstream SDL and libusb projects.

The inclusion of third-party software does not imply endorsement of MacGamepadFix by the SDL or libusb projects.
