# A-Little-More

## How to compile
- Build as normal for a Pebble app. Weather comes from [Open-Meteo](https://open-meteo.com) and needs no API key. I use the local SDK because I'm really cool, but you can use the CloudPebble Codespace if you're sane: https://codespaces.new/coredevices/codespaces-pebble?quickstart=1
- `package.json` is not tracked. It needs the message keys `TEMPERATUREC`, `TEMPERATUREF`, `CONDITIONS`, `USECELSIUS`, `USEMETRIC`, `QUAD_TL`, `QUAD_TR`, `QUAD_BL`, `QUAD_BR`, and for Pebble Time 2 (`emery`) support `emery` in `targetPlatforms` plus these two font resources (files live in `resources/fonts/`):

```json
"resources": {
  "media": [
    { "type": "font", "name": "FONT_BITHAM_57_LIGHT", "file": "fonts/bitham_57_light.pbf", "targetPlatforms": ["emery"] },
    { "type": "font", "name": "FONT_GOTHIC_33",       "file": "fonts/gothic_33.pbf",       "targetPlatforms": ["emery"] }
  ]
}
```

## Pebble Time 2
Emery's 200x228 screen is 1.357x taller than the 144x168 originals. The face keeps the exact same layout there: every vertical metric is scaled by h/168 in `main_window_load`, and the two system fonts (Bitham 42 Light, Gothic 24) are replaced by 57px / 33px copies of the same typefaces bundled as app resources. The firmware has no larger sizes of those faces, so the bundled fonts are generated from the firmware's own bitmap fonts by `tools/fontscale/` (see its README).

## TODO
- Fix "Wednesd..." (done)
- Clay config page (kinda done)
  - Temperature unit toggle (done)
  - Easier API key specification (done)
  - Colors for Basalt/Chalk/Emery?
  - Inversion for Aplite/Diorite/Flint?
- Figure out where to put weather conditions (done)
  - Because we do fetch them (yeah)
  - And then do literally nothing with them (tru!)
  - So once I figure that out, stick in a toggle for it on the Clay page (done)
 
## FAQ
### This face sucks!
yea
