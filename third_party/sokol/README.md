# sokol_gfx

`sokol_gfx.h` from https://github.com/floooh/sokol at commit
`bbac2ef1bf9d0240225e7507071cc6c08482f965` (sha256
`08743c84bb3b3dbf2bf52551cbf6e90060b4f629887cbcbdfd9b6cbb7e6031a4`), zlib license (header comment).
Unmodified. Raven compiles it once, in `src/render/sokol/sg_impl.c`, with every public
function renamed by `src/render/sokol/sg_prefix.h` (`sg_draw` becomes `rvn_sokol_sg_draw`), so an
application that links its own sokol cannot clash with it.

Upgrading is a dedicated task: sokol_gfx changes its API often. Replace the file, regenerate
the prefix header (`src/render/sokol/gen_prefix.py`), build with `-DRAVEN_BACKEND_SOKOL=ON`, run
`test_render` on Sokol, and update the commit and hash above.
