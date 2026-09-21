#!/bin/sh
# Regenerates resources/fonts/*.pbf (the Emery / Pebble Time 2 fonts) from the
# firmware's own system-font bitmaps. See README.md in this directory.
set -e
cd "$(dirname "$0")"

if [ ! -x .venv/bin/python ]; then
  python3 -m venv .venv
  .venv/bin/pip install -q -r requirements.txt
fi

BASE=https://raw.githubusercontent.com/coredevices/PebbleOS/main/resources
mkdir -p src
[ -f src/BITHAM_42_LIGHT.pbf ] || curl -sSL -o src/BITHAM_42_LIGHT.pbf "$BASE/normal/base/pbf/BITHAM_42_LIGHT.pbf"
[ -f src/GOTHIC_24.pbf ]       || curl -sSL -o src/GOTHIC_24.pbf       "$BASE/common/base/pbf/GOTHIC_24.pbf"

# 144x168 -> 200x228 is x1.357 vertically: 42px -> 57px, 24px -> 33px.
.venv/bin/python upscale_font.py src/BITHAM_42_LIGHT.pbf ../../resources/fonts/bitham_57_light.pbf 57 auto
.venv/bin/python upscale_font.py src/GOTHIC_24.pbf       ../../resources/fonts/gothic_33.pbf       33 auto
