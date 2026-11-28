#!/usr/bin/env python3
"""Writes the seed corpora of the fuzzers (tests/fuzz/corpus/<target>/).

Every seed is made here, from nothing: small documents, images and fonts
written byte by byte or with fontTools (pip install fonttools; without it
the fonts are left as they are). Nothing is taken from elsewhere, so the
corpus has no license of its own. Run it again after changing it:

    python3 tests/fuzz/make_corpus.py

Inputs that made a fuzzer crash are added next to these by hand (named
crash-<what>), and are kept: they are the regression tests of the bugs.
"""

import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS = os.path.join(HERE, "corpus")


def write(target, name, data):
    folder = os.path.join(CORPUS, target)
    os.makedirs(folder, exist_ok=True)
    if isinstance(data, str):
        data = data.encode("utf-8")
    with open(os.path.join(folder, name), "wb") as f:
        f.write(data)


# ---------------------------------------------------------------------------
#  XML
# ---------------------------------------------------------------------------

def xml():
    write("xml", "form.xml", """<VBox Name="root" Spacing="8" Slot.Padding="16">
	<Label Name="title" Text="Hello &amp; welcome" FontSize="20"/>
	<HBox Spacing="4">
		<TextInput Name="name" Hint="Your name" Text="&#x263A; &#9731;"/>
		<Button Name="go" Text="Go" OnClicked="greet"/>
	</HBox>
	<CheckBox Text="Remember" Checked="true"/>
	<Slider Min="0" Max="100" Value="42"/>
	<!-- a comment -->
	<ComboBox Items="One;Two;Three" Selected="1"/>
</VBox>
""")
    write("xml", "window.xml", """<?xml version="1.0" encoding="UTF-8"?>
<Window Title="Fuzz" Width="320" Height="200">
	<Grid Columns="2" Rows="2">
		<Label Text="A" Grid.Column="0"/>
		<Label Text="B" Grid.Column="1" Grid.Row="1"/>
	</Grid>
</Window>
""")
    write("xml", "script.xml", """<VBox>
	<Script><![CDATA[
let count: number = 0;
fn app.more() {
    count += 1;
    ui.setText("value", count.toString());
}
]]></Script>
	<Label Name="value" Text="0"/>
	<Button Text="+" OnClicked="app.more"/>
</VBox>
""")
    write("xml", "widgets.xml", """<TabView>
	<Tab Title="Tree"><TreeView><TreeItem Text="a"><TreeItem Text="b"/></TreeItem></TreeView></Tab>
	<Tab Title="Text"><TextEditor Language="c" Text="int main(void) { return 0; }"/></Tab>
	<Tab Title="Table"><Table Columns="Name;Size"><Row>x;1</Row><Row>y;2</Row></Table></Tab>
	<Tab Title="Diff"><DiffView Left="a&#10;b&#10;" Right="a&#10;c&#10;" Mode="Split"/></Tab>
	<Tab Title="Markdown"><Markdown Text="# Title&#10;**bold** _it_ `code`"/></Tab>
</TabView>
""")
    write("xml", "bad.xml", "<A><B attr='1' attr2=\"x\"></A><!-- <C>")
    write("xml", "entities.xml", "<Label Text='&lt;&gt;&amp;&quot;&apos;&#0;&#xFFFF;&#1114111;&unknown;'/>")


# ---------------------------------------------------------------------------
#  SVG
# ---------------------------------------------------------------------------

def svg():
    write("svg", "icon.svg", """<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M12 6v6l4 2"/></svg>
""")
    write("svg", "shapes.svg", """<svg xmlns="http://www.w3.org/2000/svg" width="64" height="48">
<defs>
<linearGradient id="g" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#f00"/><stop offset="1" stop-color="blue" stop-opacity="0.5"/></linearGradient>
<radialGradient id="r" cx="50%" cy="50%" r="50%" gradientTransform="rotate(30)"><stop offset="0" stop-color="#0f0"/><stop offset="1" stop-color="#000"/></radialGradient>
</defs>
<rect x="2" y="2" width="30" height="20" rx="4" fill="url(#g)"/>
<ellipse cx="45" cy="30" rx="12" ry="8" fill="url(#r)" transform="skewX(10) scale(0.9)"/>
<polygon points="5,40 20,30 35,45" fill="#123" fill-rule="evenodd" opacity=".7"/>
<polyline points="40,5 50,15 60,5" stroke="black" stroke-width="3" stroke-dasharray="2 1" fill="none"/>
<path d="M10 10 Q 20 0 30 10 T 50 10 C 55 20 60 20 62 30 A 5 5 0 0 1 50 40 Z m 2 2 h 4 v 4 l -2 2 z" fill="#808080"/>
<line x1="0" y1="47" x2="64" y2="47" stroke="#f0f"/>
<g transform="translate(5 5) matrix(1 0 0 1 2 2)" style="fill:#abc;stroke:none"><circle cx="5" cy="5" r="3"/></g>
</svg>
""")
    write("svg", "units.svg", '<svg width="2in" height="1cm" viewBox="0 0 10 5" preserveAspectRatio="xMidYMid slice"><rect width="100%" height="50%" fill="rgb(10%, 20, 30)"/></svg>')


# ---------------------------------------------------------------------------
#  Images: PNG (filters, palette, gray, 16 bits, interlaced), APNG, GIF (one
#  frame, animated), BMP, TGA, all written here
# ---------------------------------------------------------------------------

def chunk(kind, body):
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)


def png(width, height, color_type, depth, rows, plte=None, trns=None, interlace=0, extra=b""):
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, depth, color_type, 0, 0, interlace))
    if plte:
        data += chunk(b"PLTE", plte)
    if trns:
        data += chunk(b"tRNS", trns)
    data += extra
    data += chunk(b"IDAT", zlib.compress(rows))
    return data + chunk(b"IEND", b"")


def images():
    w, h = 4, 3
    rgba_rows = b"".join(bytes([f]) + bytes((x * 60 + y * 20 + c * 7) & 255 for x in range(w) for c in range(4)) for y, f in zip(range(h), (0, 1, 4)))
    write("image", "rgba.png", png(w, h, 6, 8, rgba_rows))
    write("image", "gray16.png", png(2, 2, 0, 16, b"\x00\x12\x34\xff\xff\x02\x00\x00\x80\x00"))
    write("image", "palette.png", png(3, 2, 3, 4, b"\x00\x01\x20\x00\x21\x00", plte=b"\xff\x00\x00\x00\xff\x00\x00\x00\xff", trns=b"\x80"))
    rows = b"".join(b"\x00" + bytes((x * 30 + y) & 255 for x in range(8 * 3)) for y in range(8))
    # Adam7: the seven passes of an 8 x 8 picture, rows of each (filter byte, then RGB)
    passes = [(1, 1), (1, 1), (2, 1), (2, 2), (4, 2), (4, 4), (8, 4)]
    adam7 = b"".join(bytes([k % 5]) + bytes((x * 40 + k * 9) & 255 for x in range(pw * 3)) for k, (pw, ph) in enumerate(passes) for _ in range(ph))
    write("image", "interlaced.png", png(8, 8, 2, 8, adam7, interlace=1))
    write("image", "rgb.png", png(8, 8, 2, 8, rows))

    # APNG: two frames, the second one over a part of the canvas
    actl = chunk(b"acTL", struct.pack(">II", 2, 0))
    fctl0 = chunk(b"fcTL", struct.pack(">IIIIIHHBB", 0, w, h, 0, 0, 1, 10, 0, 0))
    frame0 = rgba_rows
    fctl1 = chunk(b"fcTL", struct.pack(">IIIIIHHBB", 1, 2, 2, 1, 1, 0, 0, 1, 1))
    frame1 = b"\x00" + bytes(range(8)) + b"\x00" + bytes(range(8, 16))
    fdat = chunk(b"fdAT", struct.pack(">I", 2) + zlib.compress(frame1))
    apng = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) + actl + fctl0
    apng += chunk(b"IDAT", zlib.compress(frame0)) + fctl1 + fdat + chunk(b"IEND", b"")
    write("image", "anim.apng", apng)

    # GIF: LZW of a few pixels, with clear codes (code size 2)
    def lzw(indices, min_size=2):
        clear, end = 1 << min_size, (1 << min_size) + 1
        size = min_size + 1
        out, bits, nbits = bytearray(), 0, 0

        def put(code):
            nonlocal bits, nbits
            bits |= code << nbits
            nbits += size
            while nbits >= 8:
                out.append(bits & 255)
                bits >>= 8
                nbits -= 8
        # (a clear code before every pixel: valid, and simple)
        for i in indices:
            put(clear)
            put(i)
        put(end)
        if nbits:
            out.append(bits & 255)
        data = bytes([min_size])
        for i in range(0, len(out), 255):
            part = out[i:i + 255]
            data += bytes([len(part)]) + part
        return data + b"\x00"

    palette = b"\x00\x00\x00\xff\xff\xff\xff\x00\x00\x00\x00\xff"
    screen = b"GIF89a" + struct.pack("<HHBBB", 3, 2, 0xF1, 0, 0) + palette
    image = lambda x, y, iw, ih, px, gce: gce + b"\x2c" + struct.pack("<HHHHB", x, y, iw, ih, 0) + lzw(px)
    gce = lambda delay, dispose, transparent: b"\x21\xf9\x04" + bytes([(dispose << 2) | (1 if transparent is not None else 0)]) + struct.pack("<H", delay) + bytes([transparent or 0]) + b"\x00"
    loop = b"\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00"
    write("image", "still.gif", screen + image(0, 0, 3, 2, [0, 1, 2, 3, 2, 1], b"") + b"\x3b")
    write("image", "anim.gif", screen + loop + image(0, 0, 3, 2, [0, 1, 2, 3, 2, 1], gce(5, 1, None)) +
          image(1, 0, 2, 2, [3, 3, 0, 1], gce(10, 2, 0)) + image(0, 1, 3, 1, [2, 2, 2], gce(0, 3, None)) + b"\x3b")

    # BMP 24 bits, 2 x 2 (rows of 4 bytes, bottom up)
    pixels = b"\x00\x00\xff\x00\xff\x00\x00\x00" + b"\xff\x00\x00\xff\xff\xff\x00\x00"
    header = b"BM" + struct.pack("<IHHI", 54 + len(pixels), 0, 0, 54) + struct.pack("<IiiHHIIiiII", 40, 2, 2, 1, 24, 0, len(pixels), 2835, 2835, 0, 0)
    write("image", "small.bmp", header + pixels)
    # TGA, run-length encoded, 32 bits
    tga = struct.pack("<BBBHHBHHHHBB", 0, 0, 10, 0, 0, 0, 0, 0, 3, 2, 32, 8)
    tga += b"\x82\x10\x20\x30\xff" + b"\x02\x01\x02\x03\x80\x04\x05\x06\x40\x07\x08\x09\x00"
    write("image", "rle.tga", tga)
    # JPEG: with Pillow, when it is there (baseline and progressive)
    try:
        import io
        from PIL import Image
        picture = Image.new("RGB", (16, 8))
        picture.putdata([((x * 16) & 255, (y * 32) & 255, ((x + y) * 8) & 255) for y in range(8) for x in range(16)])
        for name, options in (("baseline.jpg", {}), ("progressive.jpg", {"progressive": True}), ("gray.jpg", {})):
            buffer = io.BytesIO()
            (picture.convert("L") if name == "gray.jpg" else picture).save(buffer, "JPEG", quality=50, **options)
            write("image", name, buffer.getvalue())
    except ImportError:
        print("Pillow is not installed: the JPEG seeds are left as they are", file=sys.stderr)


# ---------------------------------------------------------------------------
#  PNG round trip: 2 bytes of size, then pixels
# ---------------------------------------------------------------------------

def png_roundtrip():
    write("png", "gradient", bytes([7, 5]) + bytes(range(256)))
    write("png", "flat", bytes([31, 2]) + b"\x10\x20\x30\xff")
    write("png", "alpha", bytes([3, 3]) + b"\xff\x00\x00\x00\x00\xff\x00\x80\x00\x00\xff\xff")


# ---------------------------------------------------------------------------
#  Diffs
# ---------------------------------------------------------------------------

def diffs():
    write("diff", "git.diff", """diff --git a/src/app.c b/src/app.c
index 83db48f..bf269f4 100644
--- a/src/app.c
+++ b/src/app.c
@@ -1,6 +1,7 @@ int main(void)
 #include <stdio.h>

-int main(void) {
+int main(int argc, char** argv) {
+\t(void)argc;
 \tprintf("hello\\n");
 \treturn 0;
 }
@@ -10 +11,0 @@
-// end
diff --git a/notes.txt b/notes.txt
new file mode 100644
--- /dev/null
+++ b/notes.txt
@@ -0,0 +1,2 @@
+one
+two
\\ No newline at end of file
diff --git a/logo.png b/logo.png
Binary files a/logo.png and b/logo.png differ
diff --git a/run.sh b/run.sh
old mode 100644
new mode 100755
""")
    write("diff", "crlf.diff", "--- a\r\n+++ b\r\n@@ -1,2 +1,2 @@\r\n a\r\n-b\r\n+c\r\n")
    write("diff", "texts", "alpha\nbeta\ngamma\ndelta\n\x00alpha\nBETA\ngamma\ndelta\nepsilon")
    write("diff", "spaces", "a b  c\n\tx\r\ny\n\x00a b c\n  x\ny \n")


# ---------------------------------------------------------------------------
#  Raven Script
# ---------------------------------------------------------------------------

def scripts():
    write("script", "counter.rvs", """let counter: number = 0;
fn app.increment() {
    counter += 1;
    ui.setText("counter", counter.toString());
}
""")
    write("script", "fib.rvs", """fn math.fib(n: number): number {
    if (n < 2) {
        return n;
    }
    return math.fib(n - 1) + math.fib(n - 2);
}
fn app.go(): number {
    return math.fib(10);
}
""")
    write("script", "loops.rvs", """fn app.example() {
    let numbers: number[] = [1, 2, 3];
    for (let i = 0; i < 10; i += 1) {
        if (i == 5) {
            break;
        }
        numbers.push(i);
        app.log(numbers[i % 3].toString());
    }
    while (numbers.length > 0) {
        numbers.pop();
    }
}
""")
    write("script", "strings.rvs", """let s = "a,b,c";
fn app.text(): string {
    let parts = s.split(",");
    return parts[0].toUpperCase() + s.substring(1, 3) + number.parse("4").toString();
}
// a comment
/* and another */
fn app.flags(on: bool, name: string, values: number[]): bool {
    return !on || name == "x" && values.length != 0x10 || 1e3 > 2.5;
}
""")


# ---------------------------------------------------------------------------
#  Fonts (fontTools): TrueType, CFF, a collection, COLR v0 and v1, sbix
# ---------------------------------------------------------------------------

def fonts():
    try:
        from fontTools.fontBuilder import FontBuilder
        from fontTools.pens.ttGlyphPen import TTGlyphPen
        from fontTools.pens.t2CharStringPen import T2CharStringPen
        from fontTools.ttLib import TTCollection, newTable
        from fontTools.colorLib.builder import buildCOLR, buildCPAL
        import io
    except ImportError:
        print("fontTools is not installed: the font seeds are left as they are", file=sys.stderr)
        return

    names = [".notdef", "A", "g", "square"]
    cmap = {ord("A"): "A", ord("g"): "g", 0x25A0: "square", 0x1F600: "square"}

    def draw(pen, name, cubic):
        """The outline of a glyph: lines, and a curve (quadratic in TrueType, cubic in CFF)."""
        if name == ".notdef":
            pen.moveTo((50, 0)); pen.lineTo((50, 700)); pen.lineTo((450, 700)); pen.lineTo((450, 0)); pen.closePath()
        elif name == "A":
            pen.moveTo((0, 0)); pen.lineTo((250, 700)); pen.lineTo((500, 0)); pen.lineTo((400, 0)); pen.lineTo((100, 0)); pen.closePath()
        elif name == "g":
            pen.moveTo((100, -200))
            if cubic:
                pen.curveTo((400, -250), (450, 100), (300, 450))
            else:
                pen.qCurveTo((450, -200), (300, 450))
            pen.lineTo((100, 450)); pen.closePath()
        else:
            pen.moveTo((0, 0)); pen.lineTo((0, 600)); pen.lineTo((600, 600)); pen.lineTo((600, 0)); pen.closePath()

    def base(cff):
        fb = FontBuilder(1000, isTTF=not cff)
        fb.setupGlyphOrder(names)
        fb.setupCharacterMap(cmap)
        if cff:
            charstrings = {}
            for name in names:
                pen = T2CharStringPen(600, None)
                draw(pen, name, True)
                charstrings[name] = pen.getCharString()
            fb.setupCFF("FuzzSeed-Regular", {"FullName": "Fuzz Seed"}, charstrings, {})
        else:
            glyphs = {}
            for name in names:
                pen = TTGlyphPen(None)
                draw(pen, name, False)
                glyphs[name] = pen.glyph()
            fb.setupGlyf(glyphs)
        fb.setupHorizontalMetrics({name: (600, 0) for name in names})
        fb.setupHorizontalHeader(ascent=800, descent=-200)
        fb.setupNameTable({"familyName": "Fuzz Seed", "styleName": "Regular", "psName": "FuzzSeed-Regular"})
        fb.setupOS2(sTypoAscender=800, usWinAscent=800, usWinDescent=200)
        fb.setupPost()
        return fb

    def save(font):
        # (the same bytes at each run: no date of creation or of change)
        font.recalcTimestamp = False
        font["head"].created = font["head"].modified = 0x7C259DC0
        buffer = io.BytesIO()
        font.save(buffer)
        return buffer.getvalue()

    write("font", "tiny.ttf", save(base(False).font))
    write("font", "tiny.otf", save(base(True).font))

    colr0 = base(False)
    colr0.font["COLR"] = buildCOLR({"A": [("square", 0), ("A", 1)], "g": [("g", 0xFFFF)]})
    colr0.font["CPAL"] = buildCPAL([[(1.0, 0.0, 0.0, 1.0), (0.0, 0.0, 1.0, 0.5)]])
    write("font", "colr0.ttf", save(colr0.font))

    colr1 = base(False)
    paint = {"Format": 1, "Layers": [
        {"Format": 10, "Paint": {"Format": 2, "PaletteIndex": 0, "Alpha": 1.0}, "Glyph": "square"},
        {"Format": 10, "Paint": {"Format": 4, "ColorLine": {"Extend": 0, "ColorStop": [
            {"StopOffset": 0.0, "PaletteIndex": 0, "Alpha": 1.0}, {"StopOffset": 1.0, "PaletteIndex": 1, "Alpha": 1.0}]},
            "x0": 0, "y0": 0, "x1": 600, "y1": 600, "x2": 0, "y2": 600}, "Glyph": "A"}]}
    colr1.font["COLR"] = buildCOLR({"A": paint}, version=1)
    colr1.font["CPAL"] = buildCPAL([[(0.0, 0.5, 0.0, 1.0), (1.0, 1.0, 0.0, 1.0)]])
    write("font", "colr1.ttf", save(colr1.font))

    # sbix: the square is a PNG picture (2 x 2) at 20 pixels per em
    try:
        from fontTools.ttLib.tables import _s_b_i_x, sbixStrike, sbixGlyph
        sbix = base(False)
        table = newTable("sbix")
        table.version, table.flags = 1, 1
        strike = sbixStrike.Strike(ppem=20, resolution=72)
        picture = png(2, 2, 6, 8, b"\x00" + b"\xff\x00\x00\xff" * 2 + b"\x00" + b"\x00\x00\xff\x80" * 2)
        for name in names:
            glyph = sbixGlyph.Glyph(glyphName=name, graphicType="png " if name == "square" else None,
                                    imageData=picture if name == "square" else b"", originOffsetX=0, originOffsetY=0)
            strike.glyphs[name] = glyph
        table.strikes[20] = strike
        sbix.font["sbix"] = table
        write("font", "sbix.ttf", save(sbix.font))
    except Exception as error:  # noqa: BLE001 - the seed is optional
        print("sbix seed skipped: %s" % error, file=sys.stderr)

    collection = TTCollection()
    collection.fonts = [base(False).font, base(True).font]
    for font in collection.fonts:
        font.recalcTimestamp = False
        font["head"].created = font["head"].modified = 0x7C259DC0
    buffer = io.BytesIO()
    collection.save(buffer)
    write("font", "pair.ttc", buffer.getvalue())


if __name__ == "__main__":
    xml()
    svg()
    images()
    png_roundtrip()
    diffs()
    scripts()
    fonts()
