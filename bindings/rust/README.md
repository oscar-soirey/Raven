# raven-gui

Rust bindings for [Raven](https://github.com/oscar-soirey/Raven), a lightweight,
multithreaded GUI library in C for editors and tools.

```toml
[dependencies]
raven-gui = "0.1"
```

The crate is used as `raven`:

```rust
use raven::*;

fn main() {
    unsafe { Rinit(R_BACKEND_VULKAN) };
    let _window = window_from_xml(r#"<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>"#);
    action_bind("quit", |_, _| unsafe { Rquit(0) });
    while unsafe { Rwait_frame(-1.0) } != 0 {}
    unsafe { Rshutdown() };
}
```

## The Raven library

The crate links the Raven C library. Download a package for your platform
from the [releases](https://github.com/oscar-soirey/Raven/releases) (or build
Raven with `-DRAVEN_BUILD_SHARED=ON`), then tell cargo where it is:

```sh
RAVEN_LIB_DIR=/path/to/raven/lib cargo run          # Linux, macOS
set RAVEN_LIB_DIR=C:\path\to\raven\lib && cargo run   # Windows (raven.lib; raven.dll beside the program)
```

`RAVEN_STATIC=1` links the static library (`libraven.a` / `raven.lib`) instead.

`sys` is the whole C API, unchanged; the crate root adds `text`, `cstr`,
`action_bind`, `window_from_xml` and `widget_find`.

License: Apache 2.0.
