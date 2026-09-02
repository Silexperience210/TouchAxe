"""
ORDNANCE — TouchAxe visual system primitives.
All artwork is generated procedurally. No external imagery.
"""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math, random

F = "/mnt/skills/examples/canvas-design/canvas-fonts/"

FONTS = {
    "display":  F + "BigShoulders-Bold.ttf",
    "display_r":F + "BigShoulders-Regular.ttf",
    "mono":     F + "GeistMono-Regular.ttf",
    "mono_b":   F + "GeistMono-Bold.ttf",
    "tech":     F + "Jura-Medium.ttf",
    "tech_l":   F + "Jura-Light.ttf",
    "code":     F + "JetBrainsMono-Regular.ttf",
}

# ── palette ───────────────────────────────────────────────────────────────────
VOID   = (5, 7, 10)
STEEL  = (13, 19, 24)
PLATE  = (18, 26, 32)
WIRE   = (30, 42, 50)
GREY   = (85, 102, 111)
BONE   = (220, 228, 232)
AMBER  = (255, 176, 32)
ORANGE = (247, 147, 26)
EMBER  = (255, 107, 26)
PHOS   = (70, 224, 160)
ALERT  = (255, 59, 48)


def f(name, size):
    return ImageFont.truetype(FONTS[name], size)


def canvas(w, h, bg=VOID):
    return Image.new("RGB", (w, h), bg)


def mix(c1, c2, t):
    return tuple(int(a + (b - a) * t) for a, b in zip(c1, c2))


def text(d, xy, s, font, fill, anchor="la", tracking=0):
    """Draw text with optional letter-spacing (tracking in px)."""
    if tracking == 0:
        d.text(xy, s, font=font, fill=fill, anchor=anchor)
        return
    widths = [d.textlength(ch, font=font) for ch in s]
    total = sum(widths) + tracking * (len(s) - 1)
    x, y = xy
    if anchor[0] == "m":
        x -= total / 2
    elif anchor[0] == "r":
        x -= total
    va = "a" if len(anchor) < 2 else anchor[1]
    for ch, w in zip(s, widths):
        d.text((x, y), ch, font=font, fill=fill, anchor="l" + va)
        x += w + tracking


# ── structural marks ──────────────────────────────────────────────────────────
def brackets(d, box, arm, w=2, fill=WIRE, corners="all"):
    """Corner brackets — the frame language of the system."""
    x0, y0, x1, y1 = box
    c = {"tl": (x0, y0, 1, 1), "tr": (x1, y0, -1, 1),
         "bl": (x0, y1, 1, -1), "br": (x1, y1, -1, -1)}
    keys = c.keys() if corners == "all" else corners
    for k in keys:
        x, y, sx, sy = c[k]
        d.line([(x, y), (x + sx * arm, y)], fill=fill, width=w)
        d.line([(x, y), (x, y + sy * arm)], fill=fill, width=w)


def ruler(d, x0, y, x1, step, major, minor_h, major_h, fill=WIRE, w=1, vertical=False):
    """A measurement rule. The system counts, always."""
    n = 0
    p = x0
    while p <= x1:
        h = major_h if n % major == 0 else minor_h
        if vertical:
            d.line([(y, p), (y + h, p)], fill=fill, width=w)
        else:
            d.line([(p, y), (p, y + h)], fill=fill, width=w)
        p += step
        n += 1


def arc_gapped(d, box, segments, w, fill):
    """Reticle ring: arcs separated by voids."""
    for a0, a1 in segments:
        d.arc(box, a0, a1, fill=fill, width=w)


def grid(img, step, fill, alpha=1.0, offset=(0, 0)):
    ov = Image.new("RGBA", img.size, (0, 0, 0, 0))
    dd = ImageDraw.Draw(ov)
    a = int(255 * alpha)
    for x in range(offset[0], img.size[0], step):
        dd.line([(x, 0), (x, img.size[1])], fill=fill + (a,), width=1)
    for y in range(offset[1], img.size[1], step):
        dd.line([(0, y), (img.size[0], y)], fill=fill + (a,), width=1)
    img.paste(Image.alpha_composite(img.convert("RGBA"), ov).convert("RGB"), (0, 0))


def scanlines(img, period, alpha=14):
    ov = Image.new("RGBA", img.size, (0, 0, 0, 0))
    dd = ImageDraw.Draw(ov)
    for y in range(0, img.size[1], period):
        dd.line([(0, y), (img.size[0], y)], fill=(0, 0, 0, alpha), width=1)
    return Image.alpha_composite(img.convert("RGBA"), ov).convert("RGB")


def grain(img, amount=5, seed=7):
    rnd = random.Random(seed)
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            n = rnd.randint(-amount, amount)
            r, g, b = px[x, y]
            px[x, y] = (max(0, min(255, r + n)),
                        max(0, min(255, g + n)),
                        max(0, min(255, b + n)))
    return img


def vignette(img, strength=0.55):
    w, h = img.size
    m = Image.new("L", (w, h), 0)
    dm = ImageDraw.Draw(m)
    dm.ellipse([-w * 0.35, -h * 0.45, w * 1.35, h * 1.45], fill=255)
    m = m.filter(ImageFilter.GaussianBlur(min(w, h) * 0.18))
    dark = Image.new("RGB", (w, h), (0, 0, 0))
    return Image.composite(img, Image.blend(img, dark, strength), m)


# ── the mark ──────────────────────────────────────────────────────────────────
# A double-bit axe whose blades are not drawn but *derived*: each cutting edge is
# a true circular arc centred on the origin, and each bit is filled with a
# mirrored telemetry histogram. The emblem is a reading, not a picture.
HAFT_HW, HEAD_X0, EDGE_R, HEAD_HY = 25, 40, 330, 252

SERIES_R = [0.32,0.46,0.40,0.59,0.68,0.63,0.80,0.89,0.94,1.00,0.97,
            1.00,0.90,0.95,0.82,0.74,0.78,0.63,0.53,0.59,0.43,0.35]
SERIES_L = [0.28,0.37,0.52,0.47,0.61,0.72,0.66,0.83,0.91,0.99,1.00,
            0.93,0.98,0.86,0.92,0.77,0.69,0.71,0.56,0.48,0.51,0.38]


def head_polygon(cx, cy, s=1.0, side=1):
    a_end = math.degrees(math.asin(HEAD_HY / EDGE_R))
    pts = [(HEAD_X0, -HEAD_HY)]
    steps = 96
    for i in range(steps + 1):
        a = math.radians(-a_end + (2 * a_end) * i / steps)
        pts.append((EDGE_R * math.cos(a), EDGE_R * math.sin(a)))
    pts.append((HEAD_X0, HEAD_HY))
    return [(cx + side * x * s, cy + y * s) for x, y in pts]


def draw_mark(size, with_reticle=True, tone=(AMBER, EMBER), tone2=(PHOS, PHOS)):
    """The TouchAxe mark. RGBA, square, transparent ground."""
    SS = 3
    S = size * SS
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx = cy = S / 2
    s = S / 1024.0
    sw = max(1, int(6 * s))
    RING = (44, 60, 70)

    if with_reticle:
        R = 470 * s
        arc_gapped(d, [cx - R, cy - R, cx + R, cy + R],
                   [(-80, -10), (10, 80), (100, 170), (190, 260), (280, 350)],
                   max(1, int(5 * s)), RING)
        for i in range(72):
            a = math.radians(i * 5)
            L = 26 * s if i % 6 == 0 else 11 * s
            c = GREY if i % 6 == 0 else RING
            r0 = R - 30 * s
            d.line([(cx + r0 * math.cos(a), cy + r0 * math.sin(a)),
                    (cx + (r0 - L) * math.cos(a), cy + (r0 - L) * math.sin(a))],
                   fill=c, width=max(1, int(3 * s)))
        for a_deg in (0, 90, 180, 270):
            a = math.radians(a_deg)
            r0, r1 = R + 18 * s, R + 54 * s
            d.line([(cx + r0 * math.cos(a), cy + r0 * math.sin(a)),
                    (cx + r1 * math.cos(a), cy + r1 * math.sin(a))],
                   fill=tone[0], width=max(1, int(6 * s)))

    # hexagonal die, flat-top — the ASIC under the steel
    HR = 402 * s
    hexa = [(cx + HR * math.cos(math.radians(a + 30)),
             cy + HR * math.sin(math.radians(a + 30))) for a in range(0, 360, 60)]
    d.polygon(hexa, outline=RING, width=max(1, int(5 * s)))
    for vx, vy in hexa:
        d.ellipse([vx - 7 * s, vy - 7 * s, vx + 7 * s, vy + 7 * s], fill=GREY)

    # haft — a single spine, notched at the grip
    d.rectangle([cx - HAFT_HW * s, cy - 452 * s, cx + HAFT_HW * s, cy + 452 * s],
                fill=BONE)
    d.line([(cx, cy - 452 * s), (cx, cy + 452 * s)], fill=(150, 162, 170),
           width=max(1, int(2 * s)))

    polys = [head_polygon(cx, cy, s, 1), head_polygon(cx, cy, s, -1)]
    for poly in polys:
        d.polygon(poly, outline=BONE, width=sw)

    bars = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    db = ImageDraw.Draw(bars)
    for (side, series), poly in zip(((1, SERIES_R), (-1, SERIES_L)), polys):
        layer = Image.new("RGBA", (S, S), (0, 0, 0, 0))
        dl = ImageDraw.Draw(layer)
        n = len(series)
        bh = 2 * HEAD_HY * s / n
        for i, v in enumerate(series):
            y0 = cy - HEAD_HY * s + i * bh
            y1 = y0 + bh * 0.58
            x0 = cx + side * (HEAD_X0 + 20) * s
            x1 = cx + side * (HEAD_X0 + 20 + (EDGE_R - HEAD_X0 - 32) * v) * s
            dl.rectangle([min(x0, x1), y0, max(x0, x1), y1],
                         fill=mix(tone[0], tone[1], i / (n - 1)))
        mask = Image.new("L", (S, S), 0)
        ImageDraw.Draw(mask).polygon(poly, fill=255)
        layer.putalpha(Image.composite(layer.split()[3], Image.new("L", (S, S), 0), mask))
        bars = Image.alpha_composite(bars, layer)

    img = Image.alpha_composite(img, bars)
    return img.resize((size, size), Image.LANCZOS)
