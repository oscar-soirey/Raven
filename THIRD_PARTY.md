# Third-party code

Raven has no external dependency to install. The following files are included
in the repository and compiled into the library.

| Component | Where | License | Use |
|---|---|---|---|
| stb_truetype 1.26 (Sean Barrett) | `third_party/stb/stb_truetype.h` | public domain or MIT, see `third_party/stb/LICENSE` | glyph outlines and rasterization |
| stb_image 2.30 (Sean Barrett) | `third_party/stb/stb_image.h` | public domain or MIT, see `third_party/stb/LICENSE` | PNG / JPEG / BMP / TGA decoding; optional (`RAVEN_WITH_STB_IMAGE`) |
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

`stb_truetype.h` has one change, marked `Raven:`: the CFF table of an
OpenType font is read within its length (the original reads up to 512 MB
past its start), so that a damaged `.otf` cannot make it read past the end of
the file. `font_raster.c` also checks the tables of every font and each
TrueType glyph before stb reads them, since stb trusts the offsets of the file.

On Linux and BSD, the Wayland platform opens `libwayland-client`,
`libxkbcommon` and, when they are installed, `libwayland-cursor` and
`libdecor` at run time (all MIT); none of them is needed to build.

Needed to build but not part of the repository:

| Component | License | |
|---|---|---|
| Vulkan-Headers (Khronos) | Apache 2.0 or MIT | from the Vulkan SDK, or downloaded by CMake (`RAVEN_FETCH_VULKAN_HEADERS`) |

The SPIR-V in `src/render/vulkan/generated/` is compiled from the GLSL
shaders next to it with glslang.
