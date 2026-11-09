# Tests of the C3 binding

The first program of the guide (`docs/guide.html`, "A first program"): a
window, a box, a label and a Quit button. It is written four ways, once
with each module and once each in code and in XML:

| File | Module | The window |
|---|---|---|
| `c_code.c3` | `raven::c`, the C API as it is | made in code |
| `c_xml.c3` | `raven::c` | loaded from `hello.xml` |
| `raven_code.c3` | `raven`, the C3 API | made in code |
| `raven_xml.c3` | `raven` | loaded from `hello.xml` |

Without arguments, each one opens the window. With `--test`, it runs
without a window system or a GPU (`R_BACKEND_NULL`, headless). It clicks its
own Quit button, checks what happened and prints `ok`, or exits with 1.

## Building and running

From this folder, with the shared library of Raven built (`raven.lib` and
`raven.dll` on Windows, `libraven.so` on Linux, `libraven.dylib` on macOS):

```bash
c3c compile --libdir .. --lib raven -L path/to/lib raven_code.c3 -o raven_code
./raven_code --test
```

The same goes for the other three. Run the XML ones from this folder, since
they load `hello.xml` from the current directory. The library has to be
where the program can find it: next to the program or on the `PATH` on
Windows, on `LD_LIBRARY_PATH` on Linux. You can also copy it into
`../raven.c3l/linked-libs/<platform>/` and leave out `-L`.
