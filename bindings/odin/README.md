# Raven for Odin

`raven/raven.odin` is the whole C API of [Raven](https://github.com/oscar-soirey/Raven)
as foreign declarations, under the same names. Copy the `raven` folder into
your project (or point a collection at this folder:
`-collection:ext=<Raven>/bindings/odin`, then `import "ext:raven"`):

```odin
package hello

import r "raven"

main :: proc() {
	r.Rinit(r.R_BACKEND_VULKAN)
	xml :: "<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>"
	r.Rwindow_create_from_xml(xml, len(xml))
	r.Raction_bind("quit", proc "c" (sender: r.Rid, action: cstring, user_data: rawptr) {
		r.Rquit(0)
	}, nil)
	for r.Rwait_frame(-1.0) != 0 {}
	r.Rshutdown()
}
```

It links the shared library of Raven (build Raven with
`-DRAVEN_BUILD_SHARED=ON`, or take it from a release):

    odin run . -extra-linker-flags:"-L/path/to/lib -Wl,-rpath,/path/to/lib"   # Linux, macOS
    odin run . -extra-linker-flags:"/LIBPATH:C:\path\to\lib"                  # Windows: raven.lib, raven.dll beside the program

`example/main.odin` is the program of the other bindings
(`odin run example -- --test` checks the binding without a window).

`raven_odin/` is an optional layer over this binding with Odin strings,
booleans, enums, results instead of out-parameters and callbacks without
`proc "c"`; see [raven_odin/README.md](raven_odin/README.md).

Odin has no package registry: its authors choose not to have one. Packages
are folders, copied or fetched with git.
