# ORDNANCE

*A visual system for instruments that must be read.*

---

## I. The philosophy

There is a kind of object that is never looked at, only *read*: the altimeter, the
sonar plot, the fire-control repeater, the reactor annunciator. Nobody admires it.
Somebody stakes something on it. ORDNANCE is a system for that class of object. It
begins from the assumption that the screen will be glanced at from two metres away,
in a garage, at three in the morning, by someone who wants one number and wants it
now. Everything that does not serve that glance is removed — and removing it is the
work. What remains must look as though it survived a long, patient, unforgiving
edit, because it did.

Space in ORDNANCE is orthogonal and unapologetic. Corners are square. There is no
rounding, no shadow, no gloss, no gradient standing in for depth. Depth comes from
one thing only: a panel of slightly lifted value laid on a near-black ground, edged
on one side by a two-pixel stripe. The stripe is the entire ornamental budget of the
system, and it is not ornament — it is a state indicator. The grid is a four-pixel
module and every offset is a multiple of it, so that alignments are not judged by
eye but guaranteed by arithmetic. Master-level restraint is the discipline here: the
composition should feel machined, not arranged.

Colour is a language with a very small vocabulary and no synonyms. Bone and grey
carry all text. Wire carries all structure — rules, brackets, graduations, the quiet
lattice that tells the eye where things are. Beyond that there are exactly four
signal colours, and each is a *claim about the machine*: phosphor means nominal,
amber means approaching a limit, ember means thermal, alert means fault. A colour
that does not encode a state is a lie, and the system does not permit it. This is
why the interface can afford to be almost entirely monochrome: when a single orange
stripe appears, it means something, and the eye goes straight to it. Painstaking
chromatic discipline is what buys that reflex.

Scale is brutally hierarchical. One number per screen is allowed to be enormous —
the aggregate, the thing you came for — and it is set in a condensed grotesque with
the density of a stencilled hull marking. Everything else is small, monospaced,
uppercase, letter-spaced, and deliberately quiet: labels are captions on an
instrument, not headlines. The distance between the largest and smallest type in the
system is close to eight to one. That gap is not stylistic; it is what lets the
screen be legible at a glance and interrogable up close, in the same composition.

Rhythm comes from measurement. The system counts, constantly and visibly:
graduations along a rule, ticks under a sparkline, corner brackets that frame a
region without enclosing it, small index numbers in the margin. These marks come
from the visual language of the ranging scale and the calibration chart, and they do
two things at once — they make the layout feel surveyed rather than decorated, and
they reward the second look. Someone who stares at the screen for thirty seconds
should discover that every mark is load-bearing. This density of small, exact,
repeated marks is the signature of the system, and it must be executed with the
patience of a draughtsman, never faked.

The emblem obeys the same law. It is a double-bit axe whose cutting edges are true
circular arcs about the origin, whose bits are filled by a mirrored histogram of
hashrate, and whose bezel carries seventy-two graduations. It is not a drawing of an
axe. It is a *reading* that happens to have the shape of one — which is the whole
argument of ORDNANCE compressed into a single mark: the instrument and the thing it
measures are the same object.

---

## II. The register

| Token | Hex | RGB565 | Role |
|---|---|---|---|
| `OX_VOID` | `#05070A` | `0x0021` | canvas / negative |
| `OX_STEEL` | `#0D1318` | `0x0883` | panel ground |
| `OX_WIRE` | `#1E2A32` | `0x1946` | structure, rules, brackets |
| `OX_GREY` | `#55666F` | `0x532D` | secondary type |
| `OX_BONE` | `#DCE4E8` | `0xDF3D` | primary type |
| `OX_AMBER` | `#FFB020` | `0xFD84` | signal / focus / load |
| `OX_ORANGE` | `#F7931A` | `0xF483` | bitcoin accent |
| `OX_EMBER` | `#FF6B1A` | `0xFB43` | thermal |
| `OX_PHOS` | `#46E0A0` | `0x4714` | nominal |
| `OX_ALERT` | `#FF3B30` | `0xF9C6` | fault |

Ten values. No eleventh. If a new meaning appears, it takes an existing colour or it
does not get one.

---

## III. Typographic order

| Role | Face | Native size | Used for |
|---|---|---|---|
| DISPLAY | condensed grotesque, bold | 58 px | one measured value per screen |
| SUBHEAD | condensed grotesque, regular | 24 px | panel titles |
| DATA | monospace | 16 px | tabular figures |
| LABEL | monospace, +8 % tracking, caps | 10 px | field captions |
| MICRO | monospace | 8 px | footer rail, index marks |

Figures are always tabular so that a changing number does not shift the layout under
the reader's eye. Units are never the same size or colour as the value they follow.

---

## IV. Laws

1. **Square corners.** `OX_PANEL_R = 0`. No exceptions.
2. **Colour encodes state.** If it does not, it is bone, grey, or wire.
3. **One hero number per screen.** The rest is small.
4. **Every offset is a multiple of 4 px.** Alignment is arithmetic, not taste.
5. **Structure before fill.** Rules, brackets and graduations are drawn first and
   own the composition; content sits inside them.
6. **Never animate a value.** Animate a transition. A number that slides is a number
   that cannot be read.
7. **Clear space around the mark is 0.25 × its height.** Minimum size 16 px, and at
   16 px the bezel is dropped.

---

## V. Manifest

| File | Size | Purpose |
|---|---|---|
| `01_mark_*.png` | 16 → 1024 | the emblem; transparent and on-void, full and compact |
| `02_boot_splash_480x272.png` | native + @4x | ships on the device |
| `03_screen_ops_480x272.png` | native + @4x | the redesigned primary screen |
| `04_glyph_set.png` | plate | the 16-glyph set, 48 px grid, 3 px stroke |
| `icons/icon_*_48.png` | 48 px | individual glyphs, transparent |
| `05_readme_banner_1600x440.png` | banner | top of `README.md` |
| `06_social_preview_1280x640.png` | OG | GitHub → Settings → Social preview |
| `07_system_plate.png` | poster | the system, documented as one plate |
| `ordnance_theme.h` | header | the tokens above, as LVGL code |

Everything here was drawn procedurally from geometry — no stock, no traced source,
no third-party imagery. The generators are reproducible: the same code emits the
same pixels, which means the system can be re-cut at any resolution without a
redraw.
