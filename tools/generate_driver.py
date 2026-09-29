#!/usr/bin/env python3

from pathlib import Path
import ast
import copy
import json
import re

ROOT = Path(__file__).resolve().parent.parent

SOURCE = ROOT / "src/driver/unity_input_v12_reference.c"
OUTPUT = ROOT / "src/driver/unity_input.c"
HEADER = ROOT / "src/driver/generated_layouts.h"

source = SOURCE.read_text()

# ------------------------------------------------------------
# Find the JSON assignment robustly.
# We deliberately do NOT try to parse the whole assignment
# with one complicated regex.
# ------------------------------------------------------------

start_match = re.search(
    r'(?m)^[ \t]*const[ \t]+char[ \t]*\*[ \t]*json[ \t]*=',
    source
)

if not start_match:
    raise SystemExit(
        "ERROR: Could not locate 'const char *json =' in v12 reference"
    )

end_match = re.search(
    r'(?m)^[ \t]*const[ \t]+MethodInfo[ \t]*\*[ \t]*registerLayout\b',
    source[start_match.end():]
)

if not end_match:
    raise SystemExit(
        "ERROR: Could not locate 'const MethodInfo *registerLayout'"
    )

start = start_match.start()
end = start_match.end() + end_match.start()

json_block = source[start_match.end():end]

# Extract every C string literal from only the JSON area.
literal_re = re.compile(r'"(?:\\.|[^"\\])*"')

tokens = literal_re.findall(json_block)

if not tokens:
    raise SystemExit(
        "ERROR: No C string literals found in v12 JSON block"
    )

parts = []

for token in tokens:
    try:
        parts.append(ast.literal_eval(token))
    except Exception as exc:
        raise SystemExit(
            "ERROR decoding C string:\n"
            + token
            + "\n"
            + str(exc)
        )

payload = "".join(parts)

try:
    original = json.loads(payload)
except Exception as exc:
    raise SystemExit(
        "ERROR: Extracted v12 layout is not valid JSON:\n"
        + str(exc)
    )


def control(layout, name):
    for item in layout["controls"]:
        if item.get("name") == name:
            return item

    raise SystemExit(
        f"ERROR: Required control missing: {name}"
    )


required = [
    "leftStick",
    "leftStick/y",
    "leftStick/up",
    "leftStick/down",
    "rightStick",
    "rightStick/y",
    "rightStick/up",
    "rightStick/down",
]

for name in required:
    control(original, name)


# ------------------------------------------------------------
# Verify that this really is our known-good v12.
# ------------------------------------------------------------

left_parent = control(original, "leftStick")
right_parent = control(original, "rightStick")

if "invertVector2" not in left_parent.get("processors", ""):
    raise SystemExit(
        "ERROR: v12 reference does not contain leftStick invertVector2"
    )

if "invertVector2" not in right_parent.get("processors", ""):
    raise SystemExit(
        "ERROR: v12 reference does not contain rightStick invertVector2"
    )

if control(original, "leftStick/y").get("processors") != "invert":
    raise SystemExit(
        "ERROR: v12 reference leftStick/y is not the known-good version"
    )

if control(original, "rightStick/y").get("processors") != "invert":
    raise SystemExit(
        "ERROR: v12 reference rightStick/y is not the known-good version"
    )

print("Known-good v12 layout verified")


# Store v12's inverted directional definitions.
left_up_inv = control(original, "leftStick/up")["parameters"]
left_down_inv = control(original, "leftStick/down")["parameters"]

right_up_inv = control(original, "rightStick/up")["parameters"]
right_down_inv = control(original, "rightStick/down")["parameters"]


def configure_stick(layout, stick, inverted):
    parent = control(layout, stick)
    y = control(layout, stick + "/y")
    up = control(layout, stick + "/up")
    down = control(layout, stick + "/down")

    if stick == "leftStick":
        inv_up = left_up_inv
        inv_down = left_down_inv
    else:
        inv_up = right_up_inv
        inv_down = right_down_inv

    if inverted:
        # Exact working v12 behaviour.
        parent["processors"] = (
            "stickDeadzone,"
            "invertVector2(invertX=false,invertY=true)"
        )

        y["processors"] = "invert"

        up["parameters"] = inv_up
        down["parameters"] = inv_down

    else:
        # Standard Xbox / Unity orientation.
        parent["processors"] = "stickDeadzone"
        y.pop("processors", None)

        # Reverse the v12 up/down transformation.
        up["parameters"] = inv_down
        down["parameters"] = inv_up


def make_variant(left_invert, right_invert):
    layout = copy.deepcopy(original)

    configure_stick(
        layout,
        "leftStick",
        left_invert
    )

    configure_stick(
        layout,
        "rightStick",
        right_invert
    )

    return layout


layouts = {
    "MGF_LAYOUT_NORMAL_NORMAL":
        make_variant(False, False),

    "MGF_LAYOUT_INVERT_NORMAL":
        make_variant(True, False),

    "MGF_LAYOUT_NORMAL_INVERT":
        make_variant(False, True),

    "MGF_LAYOUT_INVERT_INVERT":
        make_variant(True, True),
}


def c_string(layout):
    raw = json.dumps(
        layout,
        separators=(",", ":")
    )

    escaped = (
        raw
        .replace("\\", "\\\\")
        .replace('"', '\\"')
    )

    return '"' + escaped + '"'


header_lines = [
    "#ifndef MACGAMEPADFIX_GENERATED_LAYOUTS_H",
    "#define MACGAMEPADFIX_GENERATED_LAYOUTS_H",
    "",
]

for name, layout in layouts.items():
    header_lines.append(
        f"static const char {name}[] = {c_string(layout)};"
    )
    header_lines.append("")

header_lines += [
    "#endif",
    "",
]

HEADER.write_text("\n".join(header_lines))


# ------------------------------------------------------------
# Add includes BEFORE locating replacement offsets again.
# This was the bug in the first generator.
# ------------------------------------------------------------

if '#include "generated_layouts.h"' not in source:

    include_anchor = "#include <string.h>"

    if include_anchor not in source:
        raise SystemExit(
            "ERROR: #include <string.h> not found"
        )

    source = source.replace(
        include_anchor,
        include_anchor
        + "\n#include <stdlib.h>"
        + '\n#include "generated_layouts.h"',
        1
    )


# IMPORTANT:
# Recalculate positions after modifying the source.
start_match = re.search(
    r'(?m)^[ \t]*const[ \t]+char[ \t]*\*[ \t]*json[ \t]*=',
    source
)

end_match = re.search(
    r'(?m)^[ \t]*const[ \t]+MethodInfo[ \t]*\*[ \t]*registerLayout\b',
    source[start_match.end():]
)

if not start_match or not end_match:
    raise SystemExit(
        "ERROR: Could not relocate JSON block after include insertion"
    )

start = start_match.start()
end = start_match.end() + end_match.start()


runtime_selector = '''    const char *leftYEnv =
        getenv("MGF_LEFT_Y_INVERT");

    const char *rightYEnv =
        getenv("MGF_RIGHT_Y_INVERT");

    int invertLeftY =
        leftYEnv &&
        strcmp(leftYEnv, "1") == 0;

    int invertRightY =
        rightYEnv &&
        strcmp(rightYEnv, "1") == 0;

    const char *json = NULL;

    if (invertLeftY && invertRightY) {
        json = MGF_LAYOUT_INVERT_INVERT;
    }
    else if (invertLeftY) {
        json = MGF_LAYOUT_INVERT_NORMAL;
    }
    else if (invertRightY) {
        json = MGF_LAYOUT_NORMAL_INVERT;
    }
    else {
        json = MGF_LAYOUT_NORMAL_NORMAL;
    }

    const char *verboseEnv =
        getenv("MGF_VERBOSE");

    if (verboseEnv &&
        strcmp(verboseEnv, "1") == 0) {

        fprintf(
            out,
            "MacGamepadFix 1.1.0 leftY=%s rightY=%s\\n",
            invertLeftY ? "invert" : "normal",
            invertRightY ? "invert" : "normal"
        );
    }

'''


source = (
    source[:start]
    + runtime_selector
    + source[end:]
)


# ------------------------------------------------------------
# Safety checks: do not emit a damaged source file.
# ------------------------------------------------------------

if 'FromDeviceDescription failed\\n' not in source:
    raise SystemExit(
        "ERROR: Source before JSON block was damaged"
    )

if "const MethodInfo *registerLayout" not in source:
    raise SystemExit(
        "ERROR: Source after JSON block was damaged"
    )

if source.count("const char *json = NULL;") != 1:
    raise SystemExit(
        "ERROR: Runtime selector insertion failed"
    )

if "MGF_LAYOUT_INVERT_INVERT" not in source:
    raise SystemExit(
        "ERROR: Runtime layout selection missing"
    )


OUTPUT.write_text(source)

print("")
print("MacGamepadFix driver generated successfully")
print(f"Source: {OUTPUT}")
print(f"Header: {HEADER}")
print("")
print("Layouts:")
print("  normal / normal")
print("  invert / normal")
print("  normal / invert")
print("  invert / invert")
