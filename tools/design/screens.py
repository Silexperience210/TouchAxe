"""ORDNANCE — device surfaces. Authored in native 480x272 units, rendered 4x."""
import sys, math
sys.path.insert(0, "/home/claude/gen")
from ordnance import *
from icons import render_all
from PIL import Image, ImageDraw

K = 4
Wn, Hn = 480, 272
W, H = Wn * K, Hn * K
ICONS = render_all("/home/claude/gen/out/icons")


def k(v):
    return int(round(v * K))


def fk(name, native_size):
    return f(name, int(native_size * K))


def paste_icon(img, name, x, y, size, tint=None):
    ic = ICONS[name].resize((k(size), k(size)), Image.LANCZOS)
    if tint:
        col = Image.new("RGBA", ic.size, tint + (255,))
        col.putalpha(ic.split()[3])
        ic = col
    img.paste(ic, (k(x), k(y)), ic)


# ══ BOOT SPLASH ══════════════════════════════════════════════════════════════
def boot_splash():
    img = canvas(W, H, VOID)
    grid(img, k(8), WIRE, 0.22)
    d = ImageDraw.Draw(img)

    brackets(d, (k(6), k(6), k(Wn - 6), k(Hn - 6)), k(16), K, WIRE)

    mark = draw_mark(k(112))
    img.paste(mark, (k(38), k(66)), mark)

    text(d, (k(172), k(84)), "TOUCHAXE", fk("display", 54), BONE, tracking=k(1.5))
    d.line([(k(174), k(140)), (k(444), k(140))], fill=WIRE, width=K)
    text(d, (k(174), k(148)), "PLEBARIAN MINING TERMINAL", fk("mono", 10), GREY,
         tracking=k(1.6))
    text(d, (k(174), k(166)), "SHA-256  ·  MULTI-RIG  ·  ESP32-S3", fk("mono", 9),
         AMBER, tracking=k(1.2))

    # boot progress — a rule that fills, not a bar that slides
    y = k(216)
    d.line([(k(38), y), (k(442), y)], fill=WIRE, width=K)
    ruler(d, k(38), y - k(5), k(442), k(8), 5, k(3), k(5), WIRE)
    d.line([(k(38), y), (k(38 + 268), y)], fill=AMBER, width=k(2))
    d.rectangle([k(38 + 264), y - k(4), k(38 + 268), y + k(4)], fill=AMBER)

    text(d, (k(38), k(228)), "INIT DISPLAY / TOUCH / NET", fk("mono", 8), GREY, tracking=k(1))
    text(d, (k(442), k(228)), "v1.2.0", fk("mono", 8), GREY, anchor="ra", tracking=k(1))
    text(d, (k(38), k(246)), "SILEXPERIENCE", fk("mono", 8), WIRE, tracking=k(2))
    text(d, (k(442), k(246)), "LVGL 9.4 · MIT", fk("mono", 8), WIRE, anchor="ra",
         tracking=k(1))

    img = scanlines(img, K, 12)
    return img


# ══ OPS SCREEN ═══════════════════════════════════════════════════════════════
SPARK = [.42,.51,.47,.63,.58,.71,.66,.79,.74,.88,.81,.93,.86,.97,.90,1.0,
         .94,.88,.92,.83,.87,.79,.84,.76,.81,.88,.83,.91,.86,.94,.89,.96]


def tile(d, box, label, value, unit, tone, fill_ratio=None):
    x0, y0, x1, y1 = box
    d.rectangle([k(x0), k(y0), k(x1), k(y1)], fill=STEEL)
    d.line([(k(x0), k(y0)), (k(x0), k(y1))], fill=tone, width=k(2))
    text(d, (k(x0 + 8), k(y0 + 5)), label, fk("mono", 8), GREY, tracking=k(1.2))
    text(d, (k(x0 + 8), k(y0 + 15)), value, fk("display", 22), BONE)
    vw = d.textlength(value, font=fk("display", 22))
    text(d, (k(x0 + 11) + vw, k(y0 + 24)), unit, fk("mono", 8), GREY)
    if fill_ratio is not None:
        by = k(y1 - 7)
        d.line([(k(x0 + 8), by), (k(x1 - 8), by)], fill=WIRE, width=k(2))
        d.line([(k(x0 + 8), by), (k(x0 + 8) + (k(x1 - 8) - k(x0 + 8)) * fill_ratio, by)],
               fill=tone, width=k(2))


def ops_screen():
    img = canvas(W, H, VOID)
    grid(img, k(8), WIRE, 0.18)
    d = ImageDraw.Draw(img)

    # ── status rail ───────────────────────────────────────────────────────────
    d.rectangle([0, 0, W, k(20)], fill=STEEL)
    d.line([(0, k(20)), (W, k(20))], fill=WIRE, width=K)
    sm = draw_mark(k(14), with_reticle=False)
    img.paste(sm, (k(6), k(3)), sm)
    text(d, (k(24), k(5)), "TOUCHAXE", fk("mono_b", 9), BONE, tracking=k(1.4))
    text(d, (k(96), k(6)), "OPS", fk("mono", 8), AMBER, tracking=k(1.2))
    d.rectangle([k(92), k(4), k(118), k(16)], outline=AMBER, width=K)

    text(d, (k(200), k(6)), "192.168.1.44", fk("mono", 8), GREY, tracking=k(0.8))
    paste_icon(img, "wifi", 296, 4, 13, PHOS)
    text(d, (k(316), k(6)), "-52dBm", fk("mono", 8), GREY)
    paste_icon(img, "uptime", 370, 4, 13, GREY)
    text(d, (k(474), k(4)), "14:32", fk("mono_b", 11), BONE, anchor="ra")

    # ── primary readout ───────────────────────────────────────────────────────
    d.rectangle([k(6), k(28), k(280), k(178)], fill=STEEL)
    brackets(d, (k(6), k(28), k(280), k(178)), k(12), K, WIRE)
    text(d, (k(16), k(34)), "AGGREGATE HASHRATE", fk("mono", 8), GREY, tracking=k(1.4))
    d.line([(k(16), k(48)), (k(270), k(48))], fill=WIRE, width=K)

    text(d, (k(16), k(52)), "2.41", fk("display", 58), BONE)
    vw = d.textlength("2.41", font=fk("display", 58))
    text(d, (k(24) + vw, k(66)), "TH/s", fk("display", 24), AMBER)
    text(d, (k(26) + vw, k(88)), "+3.8%", fk("mono", 9), PHOS)

    # sparkline — 60 min window
    sx0, sx1, sy0, sy1 = 16, 270, 120, 156
    d.rectangle([k(sx0), k(sy0), k(sx1), k(sy1)], fill=(9, 13, 17))
    n = len(SPARK)
    step = (sx1 - sx0) / (n - 1)
    pts = [(k(sx0 + i * step), k(sy1 - (sy1 - sy0 - 4) * v)) for i, v in enumerate(SPARK)]
    d.polygon(pts + [(k(sx1), k(sy1)), (k(sx0), k(sy1))], fill=(28, 22, 12))
    d.line(pts, fill=AMBER, width=k(1.5), joint="curve")
    d.ellipse([pts[-1][0] - k(2.5), pts[-1][1] - k(2.5),
               pts[-1][0] + k(2.5), pts[-1][1] + k(2.5)], fill=AMBER)
    ruler(d, k(sx0), k(sy1), k(sx1), k(step), 8, k(2), k(4), WIRE)
    text(d, (k(sx0 + 4), k(sy1 + 5)), "-60 MIN", fk("mono", 7), GREY, tracking=k(1))
    text(d, (k(sx1 - 4), k(sy1 + 5)), "NOW", fk("mono", 7), GREY, anchor="ra", tracking=k(1))

    # ── metric column ─────────────────────────────────────────────────────────
    tile(d, (288, 28, 474, 76), "THERMAL   MAX", "58", "°C", EMBER, 0.58)
    paste_icon(img, "temp", 452, 34, 16, WIRE)
    tile(d, (288, 80, 474, 128), "POWER   DRAW", "74.2", "W", AMBER, 0.71)
    paste_icon(img, "power", 452, 86, 16, WIRE)
    tile(d, (288, 132, 474, 178), "EFFICIENCY", "30.7", "J/TH", PHOS, 0.34)
    paste_icon(img, "efficiency", 452, 138, 16, WIRE)

    # ── rig roster ────────────────────────────────────────────────────────────
    d.line([(k(6), k(186)), (k(474), k(186))], fill=WIRE, width=K)
    text(d, (k(6), k(190)), "RIG ROSTER", fk("mono", 8), GREY, tracking=k(1.4))
    text(d, (k(470), k(190)), "4 ONLINE / 4", fk("mono", 8), PHOS, anchor="ra",
         tracking=k(1))

    rigs = [("AXE-01", "612", 0.94, PHOS), ("AXE-02", "598", 0.91, PHOS),
            ("AXE-03", "641", 0.99, PHOS), ("AXE-04", "559", 0.83, AMBER)]
    bw, gap = 112, 5
    for i, (name, hr, ratio, st) in enumerate(rigs):
        x0 = 6 + i * (bw + gap)
        x1 = x0 + bw
        d.rectangle([k(x0), k(202), k(x1), k(248)], fill=STEEL)
        d.rectangle([k(x0), k(202), k(x1), k(204)], fill=st)
        text(d, (k(x0 + 7), k(209)), name, fk("mono_b", 9), BONE, tracking=k(0.8))
        d.ellipse([k(x1 - 13), k(210), k(x1 - 8), k(215)], fill=st)
        text(d, (k(x0 + 7), k(222)), hr, fk("display", 20), BONE)
        vw = d.textlength(hr, font=fk("display", 20))
        text(d, (k(x0 + 10) + vw, k(230)), "GH/s", fk("mono", 7), GREY)
        by = k(242)
        d.line([(k(x0 + 7), by), (k(x1 - 7), by)], fill=WIRE, width=k(2))
        d.line([(k(x0 + 7), by), (k(x0 + 7) + (k(x1 - 7) - k(x0 + 7)) * ratio, by)],
               fill=st, width=k(2))

    # ── footer rail ───────────────────────────────────────────────────────────
    d.line([(k(6), k(254)), (k(474), k(254))], fill=WIRE, width=K)
    text(d, (k(6), k(258)), "BLK 892,410", fk("mono", 7), GREY, tracking=k(1))
    text(d, (k(120), k(258)), "BEST 72.6G", fk("mono", 7), AMBER, tracking=k(1))
    text(d, (k(240), k(258)), "BTC 64,182 USD", fk("mono", 7), GREY, tracking=k(1))
    text(d, (k(470), k(258)), "UP 6d 04h", fk("mono", 7), GREY, anchor="ra", tracking=k(1))

    img = scanlines(img, K, 10)
    return img


if __name__ == "__main__":
    for fn, name in ((boot_splash, "02_boot_splash"), (ops_screen, "03_screen_ops")):
        big = fn()
        big.save(f"/home/claude/gen/out/{name}_480x272@4x.png")
        big.resize((Wn, Hn), Image.LANCZOS).save(
            f"/home/claude/gen/out/{name}_480x272.png")
    print("screens done")
