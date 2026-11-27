# Third-party code

Raven has no external dependency to install. The following files are included
in the repository and compiled into the library.

| Component | Where | License | Use |
|---|---|---|---|
| stb_truetype 1.26 (Sean Barrett) | `third_party/stb/stb_truetype.h` | public domain or MIT, see `third_party/stb/LICENSE` | glyph outlines and rasterization |
| stb_image 2.30 (Sean Barrett) | `third_party/stb/stb_image.h` | public domain or MIT, see `third_party/stb/LICENSE` | PNG / JPEG / BMP / TGA decoding; optional (`RAVEN_WITH_STB_IMAGE`) |
| nanosvg and nanosvgrast (Mikko Mononen), master of November 2026 | `third_party/nanosvg/nanosvg.h`, `nanosvgrast.h` | zlib, see `third_party/nanosvg/LICENSE.txt` | SVG parsing and rasterization; optional (`RAVEN_WITH_SVG`) |
| Lucide icons 1.76.0 (Lucide Contributors; some from Feather, Cole Bemis) | 63 icons in `third_party/lucide/icons/*.svg`, embedded as `src/resources/icons.c` by `tools/embed_icons.py` | ISC License, the ones from Feather MIT, see `third_party/lucide/LICENSE` | the icons shipped with Raven (`Ricon_get`, `raven:search` in XML) |
| Roboto Regular (Google) | `third_party/roboto/Roboto-Regular-subset.ttf`, embedded as `src/resources/default_font.c` | Apache License 2.0, see `third_party/roboto/LICENSE` | default font |
| Wayland protocol descriptions (Wayland, wayland-protocols) | `third_party/wayland-protocols/*.xml`, turned into `src/platform/wayland/wayland_protocols.h` by `tools/wayland/gen_wayland.py` | MIT, see `third_party/wayland-protocols/COPYING` | the protocols of the Wayland platform (Linux, BSD) |
| Roboto Mono Regular (The Roboto Mono Project Authors) | `third_party/roboto-mono/RobotoMono-Regular-subset.ttf`, embedded as `src/resources/mono_font.c` | SIL Open Font License 1.1, see `third_party/roboto-mono/LICENSE` | fixed-width font of the text editor (`Rfont_get_monospace`) |
| Instrument Sans Regular and Bold (The Instrument Sans Project Authors) | `tools/designer/fonts/InstrumentSans-*-subset.ttf`, embedded as `tools/designer/font_body.c`, `font_body_bold.c` | SIL Open Font License 1.1, see `tools/designer/fonts/OFL-InstrumentSans.txt` | the text of the designer (not the library) |
| Bricolage Grotesque Bold (The Bricolage Grotesque Project Authors) | `tools/designer/fonts/BricolageGrotesque-Bold-subset.ttf`, embedded as `tools/designer/font_title.c` | SIL Open Font License 1.1, see `tools/designer/fonts/OFL-BricolageGrotesque.txt` | the titles of the designer |
| JetBrains Mono Regular (The JetBrains Mono Project Authors) | `tools/designer/fonts/JetBrainsMono-Regular-subset.ttf`, embedded as `tools/designer/font_code.c` | SIL Open Font License 1.1, see `tools/designer/fonts/OFL-JetBrainsMono.txt` | the XML and the code in the designer |


The font files are subsets made with `tools/subset_font.py`: Basic Latin,
Latin-1 Supplement, Latin Extended-A and a few punctuation marks and
arrows. Hinting instructions were removed. Under the Apache License and the
Open Font License these are modified versions of the original fonts.

Both stb files are compiled as `static` functions inside a single source
file each (`src/render/font_raster.c`, `src/render/image_decode.c`), so they
cannot clash with another copy of stb in the application.

nanosvg is compiled in a single source file (`src/render/svg_decode.c`)
whose `#define`s give its public names the prefix `rvn_`, so that it cannot
clash with another copy in the application. Before it draws, `svg_decode.c`
leaves out the shapes far outside the picture and dashes much smaller than a
pixel, which a damaged file could otherwise make it draw for minutes. Its
changes are marked `Raven:`. In `nanosvgrast.h`, the active edges are sorted
with a stable merge sort and the new edges of a row merged in a batch (the
original took O(n^2) on a path of many points), in the order the original
gave, so the pixels stay the same; and the slope of an edge, where an edge
starts on its first row and where it is at each following row are kept
within a million pixels, so that they stay in the range of an int. In
`nanosvg.h`, an arc whose angle is not a number (from coordinates of 1e38)
is drawn as a line, and a fill or a stroke of `currentColor` (in any
case, as an attribute or in `style`) is flagged on its shape
(`NSVG_FLAGS_FILL_CURRENT_COLOR`, `NSVG_FLAGS_STROKE_CURRENT_COLOR`) and
parsed as black, where the original read it as an unknown color name (grey);
`svg_decode.c` draws an SVG painted only in `currentColor` in white, for the
widgets to tint it.

The icons are the SVG files of the `lucide-static` package, unchanged. Each is
the same `<svg>` element (24 x 24, no fill, a stroke of 2 in `currentColor`,
round caps and joins) around a few shapes; `tools/embed_icons.py` keeps the
shapes only, without the white space between them, and checks that the
`<svg>` element is that one. Raven puts it back when an icon is asked for.

`stb_truetype.h` has five changes, marked `Raven:`: the CFF table of an
OpenType font is read within its length (the original reads up to 512 MB
past its start), so that a damaged `.otf` cannot make it read past the end of
the file; the points of a composite glyph's components are clamped to the
range of a vertex once scaled and moved (the original converts the float to
`short` directly, undefined when a damaged glyph places them out of range);
and a font of color bitmaps without outlines (CBDT or sbix, such as Noto
Color Emoji) is opened, for its character map and metrics: Raven reads the
bitmaps itself; the 32-bit numbers of the file are assembled as unsigned
(the original shifts a byte of 128 or more 24 places as an `int`, which is
undefined); and an entry of a CFF index whose offsets go past the range of an
`int` is empty (the original subtracts them as `int`s, undefined).
`font_raster.c` also checks the tables of every font and each
TrueType glyph before stb reads them, since stb trusts the offsets of the file
(and leaves out a glyph whose last contour is one point off the curve: stb
would read the point after it, past its array).

`stb_image.h` has one change, marked `Raven:`: `stbi__getn` copies nothing
for an empty chunk, instead of calling `memcpy` with a NULL buffer (a PNG IDAT
of 0 bytes, found by UBSan with the frames of an APNG). `image_decode.c` gives it `STBI_MAX_DIMENSIONS`
(16384) and refuses, before decoding, an image of more than 2^26 pixels or
of more pixels than its file can hold (stb fills what is missing with zeros).
Its integer IDCT wraps around on the coefficients of a damaged JPEG (as its
SSE2 version does, with the same pixels): `image_decode.c` is compiled with
`-fno-sanitize=signed-integer-overflow`, so that a build with UBSan does not
report it.

On Linux and BSD, the Wayland platform opens `libwayland-client`,
`libxkbcommon` and, when they are installed, `libwayland-cursor` and
`libdecor` at run time (all MIT); none of them is needed to build.

Needed to build but not part of the repository:

| Component | License | |
|---|---|---|
| Vulkan-Headers (Khronos) | Apache 2.0 or MIT | from the Vulkan SDK, or downloaded by CMake (`RAVEN_FETCH_VULKAN_HEADERS`) |

The SPIR-V in `src/render/vulkan/generated/` is compiled from the GLSL
shaders next to it with glslang.
