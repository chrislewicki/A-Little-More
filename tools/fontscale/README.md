# fontscale — larger copies of the Pebble system fonts

The watchface is designed on a 144x168 screen with two system fonts:
`FONT_KEY_BITHAM_42_LIGHT` (Gotham Light) for the time and `FONT_KEY_GOTHIC_24`
(Raster Gothic Condensed) for the four corner slots. Pebble Time 2 (`emery`,
200x228) is 228/168 = 1.357x taller, and the firmware ships no larger sizes of
either face, so `resources/fonts/` carries the same two typefaces scaled to
57px and 33px. They are bundled only for `emery` (see `resources.media` in
`package.json`) and loaded in `load_fonts()` in `src/c/ALLRedux.c`.

The firmware does not contain the TTFs (both faces are commercial); it ships
pre-rendered bitmap fonts (`.pbf`), which are public in the
[coredevices/PebbleOS](https://github.com/coredevices/PebbleOS) repository
under `resources/normal/base/pbf/` and `resources/common/base/pbf/`. The Pebble
SDK accepts `.pbf` files directly as `font` resources, so the tooling here
enlarges those bitmaps and writes new `.pbf` files:

- `pbf.py` — reader/writer for the Pebble font format (v3, RLE4 aware).
- `upscale_font.py` — scales a `.pbf` to a new pixel height. Method `trace`
  vectorises each glyph with potrace, scales the outlines and re-rasterises
  them with 16x supersampled coverage (best for smooth faces with 2px+ strokes,
  i.e. Gotham). Method `bicubic` interpolates the bitmap and thresholds it
  (keeps the thin 1px strokes of Gothic intact). `auto` measures how faithfully
  each glyph survives a trace round-trip and picks per font, with per-glyph
  fallback.
- `regenerate.sh` — downloads the two source fonts and rebuilds both outputs.

```bash
tools/fontscale/regenerate.sh
```

To target another screen size, change the two pixel heights in
`regenerate.sh` (height x screen_height / 168) and add the matching
`SCALE_Y`-style layout scaling in the C code.
