"""ORDNANCE icon system — 16 glyphs, one grid, one stroke weight."""
import sys, math, os
sys.path.insert(0, "/home/claude/gen")
from ordnance import *
from PIL import Image, ImageDraw

G = 48          # design grid
SS = 8          # supersample
U = G * SS
W = 3 * SS      # stroke, uniform across the whole set


def new():
    im = Image.new("RGBA", (U, U), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def p(v):   # grid units -> supersampled px
    return v * SS


# ── glyphs ────────────────────────────────────────────────────────────────────
def ic_hashrate(d):
    pts = [(8, 30), (13, 30), (16, 18), (20, 34), (24, 12), (28, 30), (32, 24), (36, 30), (40, 30)]
    d.line([(p(x), p(y)) for x, y in pts], fill=BONE, width=W, joint="curve")
    d.line([(p(8), p(40)), (p(40), p(40))], fill=GREY, width=W // 2)


def ic_temp(d):
    d.rounded_rectangle([p(20), p(7), p(28), p(30)], radius=p(4), outline=BONE, width=W)
    d.ellipse([p(17), p(29), p(31), p(43)], outline=BONE, width=W)
    d.line([(p(24), p(16)), (p(24), p(35))], fill=EMBER, width=W)
    for y in (13, 18, 23):
        d.line([(p(31), p(y)), (p(36), p(y))], fill=GREY, width=W // 2)


def ic_power(d):
    d.polygon([(p(26), p(6)), (p(14), p(27)), (p(22), p(27)), (p(20), p(42)),
               (p(34), p(20)), (p(26), p(20))], outline=BONE, width=W)


def ic_efficiency(d):
    d.arc([p(8), p(8), p(40), p(40)], 130, 410, fill=BONE, width=W)
    d.line([(p(24), p(24)), (p(35), p(15))], fill=AMBER, width=W)
    d.ellipse([p(21), p(21), p(27), p(27)], fill=BONE)


def ic_wifi(d):
    for r, c in ((20, BONE), (13, BONE), (6, BONE)):
        d.arc([p(24 - r), p(26 - r), p(24 + r), p(26 + r)], 205, 335, fill=c, width=W)
    d.ellipse([p(21), p(33), p(27), p(39)], fill=BONE)


def ic_share(d):
    d.polygon([(p(24), p(8)), (p(38), p(24)), (p(24), p(40)), (p(10), p(24))],
              outline=BONE, width=W)
    d.line([(p(17), p(24)), (p(22), p(29)), (p(31), p(19))], fill=PHOS, width=W, joint="curve")


def ic_difficulty(d):
    d.ellipse([p(8), p(8), p(40), p(40)], outline=BONE, width=W)
    d.ellipse([p(17), p(17), p(31), p(31)], outline=GREY, width=W)
    d.ellipse([p(22), p(22), p(26), p(26)], fill=AMBER)
    for a in (0, 90, 180, 270):
        r = math.radians(a)
        d.line([(p(24) + p(20) * math.cos(r), p(24) + p(20) * math.sin(r)),
                (p(24) + p(26) * math.cos(r), p(24) + p(26) * math.sin(r))],
               fill=BONE, width=W)


def ic_fan(d):
    d.ellipse([p(7), p(7), p(41), p(41)], outline=GREY, width=W // 2)
    for a in (90, 210, 330):
        r = math.radians(a)
        tip = (p(24) + p(15) * math.cos(r), p(24) + p(15) * math.sin(r))
        r2 = math.radians(a + 45)
        mid = (p(24) + p(13) * math.cos(r2), p(24) + p(13) * math.sin(r2))
        d.polygon([(p(24), p(24)), tip, mid], outline=BONE, width=W)
    d.ellipse([p(20), p(20), p(28), p(28)], fill=BONE)


def ic_block(d):
    top = [(p(24), p(8)), (p(40), p(17)), (p(24), p(26)), (p(8), p(17))]
    d.polygon(top, outline=BONE, width=W)
    d.line([(p(8), p(17)), (p(8), p(31)), (p(24), p(40)), (p(40), p(31)), (p(40), p(17))],
           fill=BONE, width=W, joint="curve")
    d.line([(p(24), p(26)), (p(24), p(40))], fill=GREY, width=W)


def ic_pool(d):
    nodes = [(24, 10), (11, 32), (37, 32)]
    for a in nodes:
        for b in nodes:
            if a != b:
                d.line([(p(a[0]), p(a[1])), (p(b[0]), p(b[1]))], fill=GREY, width=W // 2)
    for x, y in nodes:
        d.ellipse([p(x - 5), p(y - 5), p(x + 5), p(y + 5)], fill=VOID, outline=BONE, width=W)


def ic_uptime(d):
    d.arc([p(8), p(8), p(40), p(40)], 0, 360, fill=BONE, width=W)
    d.arc([p(8), p(8), p(40), p(40)], -90, 150, fill=PHOS, width=W)
    d.line([(p(24), p(24)), (p(24), p(15))], fill=BONE, width=W)
    d.line([(p(24), p(24)), (p(31), p(28))], fill=BONE, width=W)


def ic_alert(d):
    d.polygon([(p(24), p(8)), (p(42), p(38)), (p(6), p(38))], outline=ALERT, width=W)
    d.line([(p(24), p(19)), (p(24), p(28))], fill=ALERT, width=W)
    d.ellipse([p(22), p(31), p(26), p(35)], fill=ALERT)


def ic_settings(d):
    for i, y in enumerate((14, 24, 34)):
        d.line([(p(8), p(y)), (p(40), p(y))], fill=GREY, width=W // 2)
        cx = (18, 30, 22)[i]
        d.rectangle([p(cx - 3), p(y - 5), p(cx + 3), p(y + 5)], fill=VOID,
                    outline=BONE, width=W)


def ic_reboot(d):
    d.arc([p(9), p(9), p(39), p(39)], 60, 360, fill=BONE, width=W)
    d.polygon([(p(30), p(4)), (p(30), p(18)), (p(41), p(11))], fill=AMBER)


def ic_btc(d):
    d.line([(p(18), p(11)), (p(18), p(37))], fill=BONE, width=W)
    d.line([(p(18), p(11)), (p(28), p(11))], fill=BONE, width=W)
    d.line([(p(18), p(24)), (p(29), p(24))], fill=BONE, width=W)
    d.line([(p(18), p(37)), (p(28), p(37))], fill=BONE, width=W)
    d.arc([p(22), p(11), p(36), p(24)], -90, 90, fill=AMBER, width=W)
    d.arc([p(22), p(24), p(37), p(37)], -90, 90, fill=AMBER, width=W)
    for x in (22, 27):
        d.line([(p(x), p(6)), (p(x), p(11))], fill=BONE, width=W)
        d.line([(p(x), p(37)), (p(x), p(42))], fill=BONE, width=W)


def ic_link(d):
    d.rounded_rectangle([p(6), p(19), p(22), p(29)], radius=p(5), outline=BONE, width=W)
    d.rounded_rectangle([p(26), p(19), p(42), p(29)], radius=p(5), outline=BONE, width=W)
    d.line([(p(20), p(24)), (p(28), p(24))], fill=PHOS, width=W)


GLYPHS = [
    ("hashrate", ic_hashrate), ("temp", ic_temp), ("power", ic_power),
    ("efficiency", ic_efficiency), ("wifi", ic_wifi), ("share", ic_share),
    ("difficulty", ic_difficulty), ("fan", ic_fan), ("block", ic_block),
    ("pool", ic_pool), ("uptime", ic_uptime), ("alert", ic_alert),
    ("settings", ic_settings), ("reboot", ic_reboot), ("btc", ic_btc),
    ("link", ic_link),
]


def render_all(outdir):
    os.makedirs(outdir, exist_ok=True)
    made = {}
    for name, fn in GLYPHS:
        im, d = new()
        fn(d)
        im = im.resize((G, G), Image.LANCZOS)
        im.save(f"{outdir}/icon_{name}_48.png")
        made[name] = im
    return made


if __name__ == "__main__":
    made = render_all("/home/claude/gen/out/icons")

    # ── contact sheet: the set presented as a plate ────────────────────────────
    CELL, COLS = 168, 4
    ROWS = 4
    PAD_X, PAD_TOP, PAD_BOT = 110, 250, 150
    Wt = PAD_X * 2 + CELL * COLS
    Ht = PAD_TOP + PAD_BOT + CELL * ROWS
    sheet = canvas(Wt, Ht, VOID)
    grid(sheet, 28, WIRE, 0.30)
    d = ImageDraw.Draw(sheet)

    text(d, (PAD_X, 92), "GLYPH SET 01", f("display", 76), BONE, tracking=3)
    text(d, (PAD_X, 176), "48 PX GRID · 3 PX STROKE · 16 MARKS",
         f("mono", 19), GREY, tracking=3)
    text(d, (Wt - PAD_X, 130), "TOUCHAXE", f("mono", 19), AMBER, anchor="ra", tracking=4)
    text(d, (Wt - PAD_X, 158), "ORDNANCE  SYS", f("mono", 19), GREY, anchor="ra", tracking=4)
    d.line([(PAD_X, 220), (Wt - PAD_X, 220)], fill=WIRE, width=2)

    for i, (name, _) in enumerate(GLYPHS):
        cx = PAD_X + (i % COLS) * CELL
        cy = PAD_TOP + (i // COLS) * CELL
        brackets(d, (cx + 8, cy + 8, cx + CELL - 8, cy + CELL - 24), 14, 2, WIRE)
        ic = made[name].resize((72, 72), Image.LANCZOS)
        sheet.paste(ic, (cx + CELL // 2 - 36, cy + CELL // 2 - 46), ic)
        text(d, (cx + CELL // 2, cy + CELL - 44), name.upper(), f("mono", 15), GREY,
             anchor="ma", tracking=2)
        text(d, (cx + 16, cy + 18), f"{i+1:02d}", f("mono", 14), WIRE)

    d.line([(PAD_X, Ht - 96), (Wt - PAD_X, Ht - 96)], fill=WIRE, width=2)
    ruler(d, PAD_X, Ht - 96, Wt - PAD_X, 14, 5, 6, 12, WIRE)
    text(d, (PAD_X, Ht - 62), "MONOCHROME BY DEFAULT — COLOUR IS RESERVED FOR STATE",
         f("mono", 18), GREY, tracking=3)

    sheet = scanlines(sheet, 3, 10)
    sheet.save("/home/claude/gen/out/04_glyph_set.png")
    print("icons done")
