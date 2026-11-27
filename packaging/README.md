# Packages of the library

What makes Raven installable by the package managers of C and C++. Every one
of them builds the library with CMake and installs what `cmake --install`
installs:

| | Installed |
|---|---|
| CMake package | `lib/cmake/raven`: `find_package(raven)`, `raven::raven` |
| pkg-config | `lib/pkgconfig/raven.pc`: `pkg-config --cflags --libs raven` |
| headers | `include/Raven.h`, `include/raven/`, `include/raven_web.h` |

| Manager | Name | Files | Install (once published) |
|---|---|---|---|
| Homebrew | `raven-gui` | [homebrew/raven-gui.rb](homebrew/raven-gui.rb) | `brew install raven-gui` |
| vcpkg | `raven-gui` | [vcpkg/ports/raven-gui](vcpkg/ports/raven-gui) | `vcpkg install raven-gui` |
| Conan | `raven-gui` | [conan](conan) | `conan install --requires=raven-gui/0.3.0` |

The name is `raven-gui` everywhere, as for the bindings
([bindings/PUBLISHING.md](../bindings/PUBLISHING.md)): `raven` is already a
formula of homebrew-core (another project). In code nothing changes:
`find_package(raven)`, `raven::raven`, `pkg-config raven`, `#include <Raven.h>`.

None of them is published yet: this folder holds what is submitted, and the
scripts that test it from this working tree before a release exists.

## pkg-config

`raven.pc` is written by [cmake/RavenPkgConfig.cmake](../cmake/RavenPkgConfig.cmake)
for the build it is installed with:

```sh
cc app.c $(pkg-config --cflags --libs raven)            # the library installed
cc app.c $(pkg-config --cflags --libs --static raven)   # with libraven.a
```

- A shared build lists what the library links with (the frameworks of macOS,
  `-lm`, `-ldl`, X11, the libraries of Windows) in `Libs.private`, which only
  `--static` reads, and adds `-DRAVEN_USE_DLL` to `Cflags` (what `raven::raven`
  gives too).
- A static build has no shared library beside it, so `Libs` itself lists them:
  `--libs` alone links `libraven.a`.
- On Linux X11 is the module `x11` (`Requires` / `Requires.private`) when the
  system has `x11.pc`, `-lX11` otherwise.
- The prefix is `${pcfiledir}/../..`: an installed tree can be moved (the
  packages of the releases are unpacked anywhere).

Where both libraries are installed in one prefix (Homebrew), `raven.pc` and the
CMake package are those of the shared library; the linker prefers it to
`libraven.a` for `-lraven`, so a program that wants the static library names
`libraven.a` itself, with the flags of `pkg-config --static --libs raven`.

The tests `package_pkgconfig` and `package_cmake` (`ctest -R package_`) install
the build into a folder of its own and build
[tests/package/consumer.c](../tests/package/consumer.c) against it with
pkg-config and with `find_package(raven)`, then run it headless. The
`packages` job of the Build workflow runs them on Linux and macOS, static and
shared, and checks the Homebrew formula (`brew style`, `brew audit`).

## Each release

1. Set the version everywhere, the packages included (it resets their
   checksums to zeros: the archive of the new tag does not exist yet):

   ```sh
   python bindings/set_version.py 0.4.0
   ```

2. Commit, tag `v0.4.0`, push the tag (the Build workflow makes the release).
3. Write the checksums of the archive GitHub makes for the tag
   (`https://github.com/oscar-soirey/Raven/archive/refs/tags/v0.4.0.tar.gz`,
   the file all three download):

   ```sh
   python packaging/update_hashes.py     # downloads it; --archive <file> takes one already downloaded
   ```

   It writes `sha256` in the formula, `SHA512` in the portfile and `sha256` in
   `conandata.yml`. Commit that on main.
4. Submit the new version to each registry (below).

Before a release, each package can be tested from the working tree (commit
first: they build `git archive HEAD`):

```sh
packaging/homebrew/test-local.sh                    # macOS or Linux with Homebrew
packaging/vcpkg/test-local.sh <vcpkg root> [triplet]
packaging/conan/test-local.sh [work folder] [-o "&:shared=True" -o "&:tools=True"]
```

## Homebrew

The formula builds `libraven.a`, then the shared library with the designer and
the theme editor, from the archive of the tag, with the Vulkan headers of the
formula `vulkan-headers` (nothing is downloaded during the build). Its test
builds a program with pkg-config (shared and `libraven.a`) and with CMake, and
runs it headless.

`packaging/homebrew/test-local.sh` makes a temporary tap (`brew tap-new
local/raven-test`, trusted with `brew trust`), copies the formula there with
the url of `git archive HEAD` and its sha256, then runs `brew style`, `brew
audit --new --strict`, `brew install --build-from-source`, `brew linkage
--test` and `brew test`; at the end it uninstalls the formula, untrusts and
removes the tap.

Publishing, either:

- **A tap of the project** (no conditions): a repository
  `oscar-soirey/homebrew-raven` with `Formula/raven-gui.rb`; users run
  `brew install oscar-soirey/raven/raven-gui`. Update the file at each release.
- **homebrew-core** (`brew install raven-gui`): the project must meet the
  [acceptable formulae](https://docs.brew.sh/Acceptable-Formulae) rules
  (notability: stars, forks, users; a stable tagged release). Fork
  Homebrew/homebrew-core, add `Formula/r/raven-gui.rb`, run
  `brew audit --new --formula raven-gui` and `brew test raven-gui`, open the
  pull request; their CI builds the bottles. Later versions: `brew bump-formula-pr
  --url <archive of the tag> raven-gui`.

## vcpkg

The port takes the archive of the tag (`vcpkg_from_github`), the Vulkan headers
of the port `vulkan-headers`, and builds static or shared as the triplet says
(`VCPKG_LIBRARY_LINKAGE`; the static C runtime with `VCPKG_CRT_LINKAGE`). The
feature `tools` adds the designer and the theme editor (in
`tools/raven-gui`). Linux needs the X11 development files of the system
(`libx11-dev`). The README, docs and bindings of the install are removed; the
licenses (LICENSE, NOTICE, THIRD_PARTY.md) make the copyright file.

`packaging/vcpkg/test-local.sh <vcpkg root> [triplet]` builds the port from
`git archive HEAD`: an overlay copy of the port whose `vcpkg_from_github(...)`
is replaced with `vcpkg_extract_source_archive(... ARCHIVE <archive>)`, with
its own install, buildtrees and packages folders (the vcpkg clone is not
changed). Then it builds `tests/package` with the toolchain of vcpkg and
`consumer.c` with pkg-config, and runs them and the tools headless.

```sh
git clone https://github.com/microsoft/vcpkg ../vcpkg && ../vcpkg/bootstrap-vcpkg.sh
packaging/vcpkg/test-local.sh ../vcpkg                     # the default triplet (static)
packaging/vcpkg/test-local.sh ../vcpkg arm64-osx-dynamic   # shared
```

Publishing:

- **The vcpkg registry** (`vcpkg install raven-gui`): fork microsoft/vcpkg,
  copy `packaging/vcpkg/ports/raven-gui` to `ports/raven-gui`, then
  `./vcpkg format-manifest ports/raven-gui/vcpkg.json` and
  `./vcpkg x-add-version raven-gui` (it writes `versions/r-/raven-gui.json`
  and `versions/baseline.json`), commit, open the pull request. Later
  versions: the same with the new port.
- **Before that, or instead**: users can point `--overlay-ports` at
  `packaging/vcpkg/ports` of a clone of Raven, or a registry of the project.

## Conan

`conanfile.py` (Conan 2) builds from the archive of `conandata.yml`, with
`vulkan-headers/1.3.290.0` from Conan Center (`raven/raven_vulkan.h` includes
`vulkan/vulkan.h`, so its headers are passed on). Options: `shared`, `fPIC`,
`tools`. Conan writes its own CMake files (`find_package(raven)`,
`raven::raven`) and `raven.pc` from `package_info`, so the recipe removes those
of the install. `test_package` builds and runs a program headless.

`packaging/conan/test-local.sh` copies the recipe with a `conandata.yml`
naming `git archive HEAD` (file://) and runs `conan create`, with a
`CONAN_HOME` of its own (the cache of the user is not touched):

```sh
pip install conan
packaging/conan/test-local.sh build/conan-test
packaging/conan/test-local.sh build/conan-test -o "&:shared=True" -o "&:tools=True"
```

Publishing on **Conan Center** (`conan install --requires=raven-gui/<version>`):
fork conan-io/conan-center-index, add `recipes/raven-gui/all/` (this
`conanfile.py` without the `version` attribute, `conandata.yml`,
`test_package/`) and `recipes/raven-gui/config.yml`
(`versions: "0.4.0": folder: all`), open the pull request; their CI builds every
configuration and their review asks for their conventions (for Linux, likely
`xorg/system` in place of the system library `X11`).
