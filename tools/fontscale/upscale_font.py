"""Scale a Pebble .pbf bitmap font to a larger pixel size.

Method "trace": vectorise every glyph with potrace, scale the outlines, and
re-rasterise them with supersampled coverage (>= 50% => ink).
Method "bicubic": interpolate the 1-bit glyph bitmap and threshold (level-set).
"""
import math, sys
import numpy as np
from PIL import Image, ImageDraw
import potrace
import pbf

SS = 16          # supersampling factor for the trace method
FLATTEN = 12     # line segments per bezier

def rnd(x):
    return int(math.floor(x + 0.5))

def glyph_contours(g):
    """Return closed polylines (in glyph-bitmap pixel coords, y down) tracing the ink."""
    arr = np.full((g.height + 2, g.width + 2), 255, np.uint8)
    rows = g.rows()
    for r, row in enumerate(rows):
        for c, bit in enumerate(row):
            if bit:
                arr[r + 1, c + 1] = 0
    bm = potrace.Bitmap(arr)              # dark pixels become the traced foreground
    path = bm.trace(turdsize=0, alphamax=1.0, opticurve=True, opttolerance=0.2)
    contours = []
    for curve in path:
        sp = curve.start_point
        pts = [(sp.x, sp.y)]
        prev = pts[0]
        for seg in curve:
            e = (seg.end_point.x, seg.end_point.y)
            if seg.is_corner:
                pts.append((seg.c.x, seg.c.y)); pts.append(e)
            else:
                c1 = (seg.c1.x, seg.c1.y); c2 = (seg.c2.x, seg.c2.y)
                for i in range(1, FLATTEN + 1):
                    t = i / FLATTEN; mt = 1 - t
                    x = mt**3*prev[0] + 3*mt*mt*t*c1[0] + 3*mt*t*t*c2[0] + t**3*e[0]
                    y = mt**3*prev[1] + 3*mt*mt*t*c1[1] + 3*mt*t*t*c2[1] + t**3*e[1]
                    pts.append((x, y))
            prev = e
        # shift out the 1px padding
        contours.append([(x - 1, y - 1) for (x, y) in pts])
    return contours

def rasterize(contours, s, ox, oy):
    """Scale contours by s, translate by (ox, oy) (line-box coords), rasterise with even-odd fill.
    Returns (ink bool array, x0, y0) where (x0, y0) is the array origin in line-box coords."""
    pts_all = [(x * s + ox, y * s + oy) for c in contours for (x, y) in c]
    if not pts_all:
        return None, 0, 0
    x0 = int(math.floor(min(p[0] for p in pts_all))) - 1
    y0 = int(math.floor(min(p[1] for p in pts_all))) - 1
    x1 = int(math.ceil(max(p[0] for p in pts_all))) + 1
    y1 = int(math.ceil(max(p[1] for p in pts_all))) + 1
    W, H = x1 - x0, y1 - y0
    acc = np.zeros((H * SS, W * SS), dtype=bool)
    for c in contours:
        poly = [((x * s + ox - x0) * SS, (y * s + oy - y0) * SS) for (x, y) in c]
        im = Image.new("1", (W * SS, H * SS), 0)
        ImageDraw.Draw(im).polygon(poly, fill=1, outline=None)
        acc ^= np.array(im, dtype=bool)
    cov = acc.reshape(H, SS, W, SS).mean(axis=(1, 3))
    return cov >= 0.5, x0, y0

def scale_glyph_trace(g, s):
    if g.width == 0 or g.height == 0 or not any(g.bits):
        return pbf.Glyph(0, 0, 0, 0, rnd(g.advance * s), [])
    contours = glyph_contours(g)
    ink, x0, y0 = rasterize(contours, s, g.left * s, g.top * s)
    return crop_to_glyph(ink, x0, y0, rnd(g.advance * s))

def scale_glyph_bicubic(g, s):
    if g.width == 0 or g.height == 0 or not any(g.bits):
        return pbf.Glyph(0, 0, 0, 0, rnd(g.advance * s), [])
    pad = 2
    src = Image.new("L", (g.width + 2 * pad, g.height + 2 * pad), 0)
    px = src.load()
    for r, row in enumerate(g.rows()):
        for c, bit in enumerate(row):
            if bit: px[c + pad, r + pad] = 255
    # exact origin of the padded image in line-box coords
    ox, oy = (g.left - pad) * s, (g.top - pad) * s
    x0, y0 = int(math.floor(ox)), int(math.floor(oy))
    fx, fy = ox - x0, oy - y0
    W = int(math.ceil(src.width * s + fx)) + 1
    H = int(math.ceil(src.height * s + fy)) + 1
    # resample with an affine transform so sub-pixel placement is preserved
    a = 1 / s
    big = src.transform((W, H), Image.AFFINE, (a, 0, -fx * a, 0, a, -fy * a), resample=Image.BICUBIC)
    ink = np.array(big) >= 128
    return crop_to_glyph(ink, x0, y0, rnd(g.advance * s))

def crop_to_glyph(ink, x0, y0, advance):
    ys, xs = np.nonzero(ink)
    if len(ys) == 0:
        return pbf.Glyph(0, 0, 0, 0, advance, [])
    by0, by1, bx0, bx1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    sub = ink[by0:by1, bx0:bx1]
    bits = [int(v) for v in sub.flatten()]
    return pbf.Glyph(int(bx1 - bx0), int(by1 - by0), int(x0 + bx0), int(y0 + by0), advance, bits)

def trace_fidelity(g):
    """Mismatch fraction and ink ratio of tracing + re-rasterising the glyph at scale 1."""
    ink, x0, y0 = rasterize(glyph_contours(g), 1.0, g.left, g.top)
    if ink is None:
        return 1.0, 0.0
    # common canvas covering both the traced result and the source glyph
    cx0, cy0 = min(x0, g.left), min(y0, g.top)
    cx1, cy1 = max(x0 + ink.shape[1], g.left + g.width), max(y0 + ink.shape[0], g.top + g.height)
    canvas_t = np.zeros((cy1 - cy0, cx1 - cx0), bool)
    canvas_t[y0 - cy0:y0 - cy0 + ink.shape[0], x0 - cx0:x0 - cx0 + ink.shape[1]] = ink
    src = np.zeros_like(canvas_t)
    for r, row in enumerate(g.rows()):
        for c, bit in enumerate(row):
            if bit:
                src[g.top + r - cy0, g.left + c - cx0] = True
    n = max(1, int(src.sum()))
    return float((canvas_t ^ src).sum()) / n, float(canvas_t.sum()) / n

def glyph_is_traceable(g):
    if g.width == 0 or g.height == 0 or not any(g.bits):
        return True
    mism, ratio = trace_fidelity(g)
    return mism <= 0.20 and 0.90 <= ratio <= 1.10

def scale_font(src, s, new_height, method="auto"):
    """method: "trace", "bicubic", or "auto".

    "auto" traces the font (best for smooth outlines with 2px+ strokes) unless
    tracing reproduces most glyphs poorly (thin 1px strokes get eroded), in
    which case the whole font uses the bicubic level-set method. Individual
    glyphs that trace badly in an otherwise traceable font fall back to bicubic.
    """
    out = pbf.PBF(); out.max_height = new_height
    uniq = {id(g): g for g in src.glyphs.values()}
    if method == "auto":
        ok = {k: glyph_is_traceable(g) for k, g in uniq.items()}
        frac = sum(ok.values()) / len(ok)
        font_method = "trace" if frac >= 0.8 else "bicubic"
        print(f"  auto: {frac:.0%} of glyphs trace faithfully -> {font_method}"
              + (f" ({sum(1 for v in ok.values() if not v)} glyphs fall back to bicubic)" if font_method == "trace" else ""))
    else:
        ok = {k: True for k in uniq}; font_method = method
    cache = {}
    for k, g in uniq.items():
        use_trace = font_method == "trace" and ok[k]
        cache[k] = scale_glyph_trace(g, s) if use_trace else scale_glyph_bicubic(g, s)
    for cp, g in src.glyphs.items():
        out.glyphs[cp] = cache[id(g)]
    return out

if __name__ == "__main__":
    src_path, dst_path, new_height, method = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
    src = pbf.read(src_path)
    s = new_height / src.max_height
    out = scale_font(src, s, new_height, method)
    n = pbf.write(dst_path, out)
    mx = max((g.width * g.height + 7) // 8 for g in out.glyphs.values())
    print(f"{src_path} ({src.max_height}px) -> {dst_path} ({new_height}px, x{s:.4f}, {method}): {len(out.glyphs)} glyphs, {n} bytes, largest glyph {mx} bytes")
