# Fuzzers

libFuzzer targets for what Raven reads from files it does not control:

| Target | What it feeds | Checks besides crashes |
|---|---|---|
| `fuzz_xml` | `Rxml_document_parse` and its writer; `Rwidget_create_from_xml`, `Rwindow_create_from_xml`, laid out and painted (headless) | a document read is written as a text that reads again, and is written the same |
| `fuzz_font` | the glue of stb_truetype (metrics, outlines, COLR layers, sbix / CBDT bitmaps, collections); `Rfont_create`, measured and drawn plain, bold and italic | |
| `fuzz_svg` | nanosvg as `svg_decode.c` drives it, at three scales | the size stays within the bounds |
| `fuzz_image` | `Rimage_decode`, `Rimage_decode_info`, `Rimage_decode_frame` (stb_image, `anim_decode.c`), and the frames of an animation composed in order | what the API says of an image is what decoding it gives |
| `fuzz_png` | `Rimage_encode_png` then `Rimage_decode` (the input is a picture: 2 bytes of size, then pixels) | the picture comes back pixel for pixel |
| `fuzz_diff` | `rvn_patch_parse` (unified patches), and `Rdiff_unified` of two texts (the input split at its first 0 byte) | the patch Raven writes reads back and, applied, gives the right text |
| `fuzz_script` | Raven Script compiled, installed, its functions run within small limits; analysis, completion, formatter | the stack is empty after each call; the formatter's output formats to itself |

## Building and running

With clang and its libFuzzer (on macOS, Homebrew's `llvm`: Apple's clang has
no libFuzzer; the null platform there keeps the SDK's Objective-C headers out):

```sh
cmake -S . -B build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang \
      -DRAVEN_PLATFORM=null -DRAVEN_BUILD_FUZZERS=ON -DRAVEN_BUILD_TESTS=OFF \
      -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TOOLS=OFF
cmake --build build-fuzz
mkdir -p /tmp/corpus-xml && cp tests/fuzz/corpus/xml/* /tmp/corpus-xml/
build-fuzz/bin/fuzz_xml /tmp/corpus-xml -max_total_time=300 -timeout=25
```

The whole build is instrumented with AddressSanitizer and UBSan, and a
fuzzing build neither downloads nor opens anything outside (`RAVEN_FUZZING`
in `core/fs.c`). A crash leaves its input in `crash-<hash>`;
`build-fuzz/bin/fuzz_xml crash-<hash>` runs it again.

CI (`.github/workflows/build.yml`, job `fuzz`) runs every target for 90
seconds on each pull request, and uploads what it finds (`fuzz-findings`).

## The corpus

`corpus/<target>/` holds small seeds, all written by `make_corpus.py` from
nothing (with fontTools and Pillow for the fonts and the JPEGs), so they
carry no license of their own; `fuzz_font` also starts from the Roboto subset
of `third_party/roboto`. Run `python3 tests/fuzz/make_corpus.py` again after
changing it.

An input that made a target crash is added there by hand (`crash-<what>`,
`timeout-<what>`) once the bug is fixed. Every file of the corpus is run once
by the test `fuzz_<target>_corpus`: with libFuzzer, and also in an ordinary
build of the tests with the static library, where `fuzz_main.c` gives the
targets a `main()` (so the sanitizer job and every platform run them too).
