# Wayland protocol files

The descriptions of the Wayland protocols Raven speaks, unchanged, MIT
licensed (see `COPYING`). `tools/wayland/gen_wayland.py` reads them and writes
`src/platform/wayland/wayland_protocols.h`; nothing here is compiled directly.

| File | From | Used for |
|---|---|---|
| `wayland.xml` | Wayland | the core protocol: surfaces, seat, outputs, clipboard and drops |
| `xdg-shell.xml` | wayland-protocols, stable | windows |
| `xdg-decoration-unstable-v1.xml` | wayland-protocols, unstable | title bars drawn by the compositor |
| `viewporter.xml` | wayland-protocols, stable | showing a buffer at the logical size of a window |
| `fractional-scale-v1.xml` | wayland-protocols, staging | fractional scales |
| `cursor-shape-v1.xml` | wayland-protocols, staging | cursors drawn by the compositor |
| `ext-background-effect-v1.xml` | wayland-protocols, staging | blur behind transparent windows |
| `xdg-toplevel-icon-v1.xml` | wayland-protocols, staging | window icons |

To speak another protocol: add its file here and to the list at the top of
`gen_wayland.py`, and run it.
