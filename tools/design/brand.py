import sys, math
sys.path.insert(0, "/home/claude/gen")
from ordnance import *
from PIL import Image, ImageDraw

OUT = "/home/claude/gen/out"


def rgb565(c):
    r, g, b = c
    return (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)


PALETTE = [
    ("VOID",   VOID,   "canvas / negative"),
    ("STEEL",  STEEL,  "panel ground"),
    ("WIRE",   WIRE,   "structure / rules"),
    ("GREY",   GREY,   "secondary type"),
    ("BONE",   BONE,   "primary type"),
    ("AMBER",  AMBER,  "signal / focus"),
    ("ORANGE", ORANGE, "bitcoin accent"),
    ("EMBER",  EMBER,  "thermal load"),
    ("PHOS",   PHOS,   "nominal state"),
    ("ALERT",  ALERT,  "fault state"),
]

# ══ 01 · the mark ════════════════════════════════════════════════════════════
m = draw_mark(1024)
m.save(f"{OUT}/01_mark_1024_transparent.png")
plate = canvas(1024, 1024, VOID)
plate.paste(m, (0, 0), m)
plate.save(f"{OUT}/01_mark_1024_onvoid.png")
draw_mark(512, with_reticle=False).save(f"{OUT}/01_mark_512_compact.png")
for s in (16, 32, 64, 128, 256):
    draw_mark(s, with_reticle=(s >= 128)).save(f"{OUT}/01_mark_{s}.png")


# ══ 05 · README banner ═══════════════════════════════════════════════════════
def banner(W=1600, H=440):
    img = canvas(W, H, VOID)
    grid(img, 20, WIRE, 0.28)
    d = ImageDraw.Draw(img)
    brackets(d, (28, 28, W - 28, H - 28), 44, 3, WIRE)

    mk = draw_mark(232)
    img.paste(mk, (86, (H - 232) // 2 - 8), mk)

    x = 360
    text(d, (x, 118), "TOUCHAXE", f("display", 132), BONE, tracking=4)
    d.line([(x + 4, 268), (W - 96, 268)], fill=WIRE, width=3)
    text(d, (x + 4, 282), "PLEBARIAN MINING TERMINAL  ·  ESP32-S3  ·  LVGL 9",
         f("mono", 26), GREY, tracking=3)
    text(d, (x + 4, 322), "MULTI-RIG TELEMETRY / SUB-MS TOUCH / OTA", f("mono", 24),
         AMBER, tracking=3)

    ruler(d, 96, H - 62, W - 96, 16, 5, 6, 12, WIRE)
    text(d, (96, H - 48), "SILEXPERIENCE", f("mono", 20), WIRE, tracking=5)
    text(d, (W - 96, H - 48), "MIT · OPEN HARDWARE", f("mono", 20), WIRE, anchor="ra",
         tracking=3)
    return scanlines(img, 3, 10)


banner().save(f"{OUT}/05_readme_banner_1600x440.png")


# ══ 06 · social preview ══════════════════════════════════════════════════════
def social(W=1280, H=640):
    img = canvas(W, H, VOID)
    grid(img, 16, WIRE, 0.26)
    d = ImageDraw.Draw(img)

    # a faint field of the telemetry motif, bled off the right edge
    ov = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    do = ImageDraw.Draw(ov)
    for i, v in enumerate(SERIES_R + SERIES_L):
        y = 40 + i * 13.6
        do.rectangle([W - 40 - 300 * v, y, W - 40, y + 7], fill=WIRE + (200,))
    img = Image.alpha_composite(img.convert("RGBA"), ov).convert("RGB")
    d = ImageDraw.Draw(img)

    brackets(d, (36, 36, W - 36, H - 36), 40, 3, WIRE)
    mk = draw_mark(210)
    img.paste(mk, (80, 96), mk)

    text(d, (78, 340), "TOUCHAXE", f("display", 150), BONE, tracking=4)
    d.line([(84, 512), (W - 300, 512)], fill=AMBER, width=4)
    text(d, (84, 528), "MULTI-RIG BITCOIN TELEMETRY FOR ESP32-S3", f("mono", 27),
         GREY, tracking=3)

    text(d, (330, 150), "SHA-256", f("mono", 24), AMBER, tracking=4)
    text(d, (330, 190), "480 × 272  ·  GT911  ·  LVGL 9.4", f("mono", 22), GREY,
         tracking=2)
    text(d, (330, 230), "OPEN SOURCE  ·  MIT", f("mono", 22), WIRE, tracking=2)

    text(d, (84, 588), "SILEXPERIENCE", f("mono", 22), WIRE, tracking=5)
    return scanlines(img, 3, 12)


social().save(f"{OUT}/06_social_preview_1280x640.png")


# ══ 07 · system plate ════════════════════════════════════════════════════════
def system_plate(W=2000, H=2990):
    img = canvas(W, H, VOID)
    grid(img, 25, WIRE, 0.24)
    d = ImageDraw.Draw(img)
    M = 130
    brackets(d, (70, 70, W - 70, H - 70), 56, 3, WIRE)

    # — head —
    text(d, (M, 168), "ORDNANCE", f("display", 176), BONE, tracking=6)
    text(d, (M, 356), "A VISUAL SYSTEM FOR INSTRUMENTS THAT MUST BE READ",
         f("mono", 28), GREY, tracking=5)
    text(d, (W - M, 190), "PLATE 01", f("mono", 26), AMBER, anchor="ra", tracking=5)
    text(d, (W - M, 232), "TOUCHAXE", f("mono", 26), GREY, anchor="ra", tracking=5)
    text(d, (W - M, 274), "REV A", f("mono", 26), WIRE, anchor="ra", tracking=5)
    d.line([(M, 412), (W - M, 412)], fill=WIRE, width=3)
    ruler(d, M, 412, W - M, 20, 5, 8, 16, WIRE)

    # — construction of the mark —
    y = 500
    text(d, (M, y), "01 — CONSTRUCTION", f("mono", 30), BONE, tracking=4)
    text(d, (M, y + 46), "THE CUTTING EDGE IS A CIRCULAR ARC ABOUT THE ORIGIN.",
         f("mono", 22), GREY, tracking=2)
    text(d, (M, y + 78), "THE BIT IS FILLED BY A MIRRORED HASHRATE HISTOGRAM.",
         f("mono", 22), GREY, tracking=2)

    specs = [("EDGE RADIUS", "330 U"), ("BIT DEPTH", "290 U"), ("HAFT WIDTH", "50 U"),
             ("HISTOGRAM", "22 BINS"), ("BEZEL", "72 GRADUATIONS"),
             ("DIE", "HEXAGON, FLAT-TOP"), ("CLEAR SPACE", "0.25 × HEIGHT"),
             ("MINIMUM SIZE", "16 PX")]
    sy = y + 168
    for i, (lbl, val) in enumerate(specs):
        ry = sy + i * 48
        d.line([(M, ry + 34), (M + 700, ry + 34)], fill=(20, 28, 34), width=2)
        text(d, (M, ry), lbl, f("mono", 21), GREY, tracking=2)
        text(d, (M + 700, ry), val, f("mono", 21), BONE, anchor="ra", tracking=2)

    mk = draw_mark(460)
    img.paste(mk, (W - M - 480, y - 40), mk)
    # dimension lines
    cx, cy = W - M - 480 + 230, y - 40 + 230
    d.line([(cx - 300, cy), (cx + 300, cy)], fill=WIRE, width=2)
    d.line([(cx, cy - 300), (cx, cy + 300)], fill=WIRE, width=2)
    for lbl, dx, dy in (("R 330", 200, -212), ("Ø 940", -250, 224)):
        text(d, (cx + dx, cy + dy), lbl, f("mono", 20), WIRE, tracking=2)

    yy = y + 560
    d.line([(M, yy), (W - M, yy)], fill=WIRE, width=3)

    # — palette —
    yy += 46
    text(d, (M, yy), "02 — CHROMATIC REGISTER", f("mono", 30), BONE, tracking=4)
    text(d, (M, yy + 44), "TEN VALUES. COLOUR CARRIES STATE, NEVER DECORATION.",
         f("mono", 22), GREY, tracking=2)
    yy += 96
    sw_w = (W - 2 * M) // 5
    for i, (name, col, role) in enumerate(PALETTE):
        cx0 = M + (i % 5) * sw_w
        cy0 = yy + (i // 5) * 300
        d.rectangle([cx0, cy0, cx0 + sw_w - 22, cy0 + 150], fill=col)
        if col in (VOID, STEEL, WIRE):
            d.rectangle([cx0, cy0, cx0 + sw_w - 22, cy0 + 150], outline=WIRE, width=2)
        text(d, (cx0, cy0 + 168), name, f("mono_b", 26), BONE, tracking=3)
        text(d, (cx0, cy0 + 202), "#%02X%02X%02X" % col, f("mono", 21), GREY, tracking=2)
        text(d, (cx0, cy0 + 232), "565 0x%04X" % rgb565(col), f("mono", 21), AMBER,
             tracking=2)
        text(d, (cx0, cy0 + 262), role.upper(), f("mono", 19), WIRE, tracking=1.5)

    yy += 640
    d.line([(M, yy), (W - M, yy)], fill=WIRE, width=3)

    # — typography —
    yy += 46
    text(d, (M, yy), "03 — TYPOGRAPHIC ORDER", f("mono", 30), BONE, tracking=4)
    yy += 60
    rows = [("DISPLAY", "display", 96, "2.41", "MEASURED VALUES ONLY"),
            ("SUBHEAD", "display_r", 54, "AGGREGATE", "PANEL TITLES"),
            ("DATA",    "mono", 40, "612 GH/s", "TABULAR FIGURES"),
            ("LABEL",   "mono", 26, "THERMAL MAX", "TRACKED +8%"),
            ("MICRO",   "mono", 20, "BLK 892,410", "FOOTER RAIL")]
    for name, fam, size, sample, note in rows:
        d.line([(M, yy), (W - M, yy)], fill=(22, 30, 36), width=2)
        text(d, (M, yy + 26), name, f("mono", 22), AMBER, tracking=3)
        text(d, (M + 300, yy + 12), sample, f(fam, size), BONE)
        text(d, (W - M, yy + 30), note, f("mono", 20), WIRE, anchor="ra", tracking=2)
        yy += size + 62

    d.line([(M, yy), (W - M, yy)], fill=WIRE, width=3)

    # — state —
    yy += 46
    text(d, (M, yy), "04 — STATE", f("mono", 30), BONE, tracking=4)
    yy += 60
    states = [("NOMINAL", PHOS), ("LOAD", AMBER), ("THERMAL", EMBER), ("FAULT", ALERT),
              ("OFFLINE", GREY)]
    bw = (W - 2 * M) // 5
    for i, (name, col) in enumerate(states):
        x0 = M + i * bw
        d.rectangle([x0, yy, x0 + bw - 24, yy + 92], fill=STEEL)
        d.rectangle([x0, yy, x0 + 8, yy + 92], fill=col)
        d.ellipse([x0 + 30, yy + 26, x0 + 54, yy + 50], fill=col)
        text(d, (x0 + 30, yy + 60), name, f("mono", 21), BONE, tracking=2)

    # — colophon —
    text(d, (M, H - 150), "SILEXPERIENCE  ·  TOUCHAXE  ·  MIT", f("mono", 22), GREY,
         tracking=5)
    text(d, (W - M, H - 150), "GENERATED, NOT SOURCED", f("mono", 22), WIRE,
         anchor="ra", tracking=3)
    ruler(d, M, H - 116, W - M, 20, 5, 8, 16, WIRE)

    img = scanlines(img, 4, 9)
    return vignette(img, 0.22)


system_plate().save(f"{OUT}/07_system_plate.png")
print("brand done")
