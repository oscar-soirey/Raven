# Packaging the bindings

| Registry | Package | Install | Contents |
|---|---|---|---|
| PyPI | `raven-gui` | `pip install raven-gui` | `raven`, with the library included |
| NuGet | `Raven.Gui` | `dotnet add package Raven.Gui` | `Raven.cs` (netstandard2.0) and the library of each platform |
| crates.io | `raven-gui` | `raven-gui = "0.1"` | used as `raven`; the library comes from a release (`RAVEN_LIB_DIR`) |
| LuaRocks | `raven-gui` | `luarocks install raven-gui` | `raven`; the library comes from a release |
| Maven Central | `io.github.oscar-soirey:raven-gui` | `implementation("io.github.oscar-soirey:raven-gui:0.2.0")` | `raven.R`, with the library of each platform included |

The names end in `-gui` because `raven` is already taken on PyPI (the old
Sentry client), and one name everywhere is simpler. Check that `raven-gui` /
`Raven.Gui` are still free on each registry before the first publication. In
code nothing changes: `import raven`, `use raven::*`, `require("raven")`,
`using Raven`, `import static raven.R.*`. On Maven Central a package belongs to
a namespace: `io.github.oscar-soirey` is the one Central gives to the GitHub
account (sign in to central.sonatype.com with GitHub and check that it is
listed under Namespaces).

## Making the packages

From the folder of Raven, in a "Developer Command Prompt for Visual Studio":

```bat
bindings\package.bat
```

It builds Raven (shared, Release) and writes, in `dist\bindings\`:

```
pypi\       raven_gui-0.2.0-py3-none-win_amd64.whl
nuget\      Raven.Gui.0.2.0.nupkg
crates\     raven-gui-0.2.0.crate
maven\      raven-gui-0.2.0.jar, -sources.jar, -javadoc.jar
luarocks\   raven-gui-0.2.0-1.rockspec
```

It publishes nothing. A part whose tool is not installed (Python, the .NET
SDK, cargo, Maven) is skipped with a message.

The wheel, the NuGet package and the jar hold the native library, so a Windows build
gives the Windows wheel and a NuGet package for Windows only. To add Linux and
macOS, unzip the shared packages of a release (made by the Build workflow) and
give them to the script:

```bat
bindings\package.bat --linux C:\tmp\raven-linux-x64-shared --macos C:\tmp\raven-macos-universal-shared
```

This adds the Linux and macOS wheels, and their libraries in the NuGet
package and the jar. `--prefix <folder>` uses an existing shared build of Raven for
Windows instead of building one.

The version is the one of `CMakeLists.txt`. To change it everywhere (CMake,
Cargo, NuGet, rockspec):

```bat
python bindings\set_version.py 0.2.0
```

## Publishing them (by hand)

Create an account on each registry first. Then, from `dist\bindings\`:

```bat
rem PyPI (pip install twine)
python -m twine upload pypi\*

rem NuGet (nuget.org > API Keys)
dotnet nuget push nuget\*.nupkg --api-key <key> --source https://api.nuget.org/v3/index.json

rem crates.io (cargo login <token> once): from the folder of the crate
cd bindings\rust && cargo publish

rem Maven Central: signed with gpg, sent with the user token of central.sonatype.com
rem (MAVEN_CENTRAL_USERNAME, MAVEN_CENTRAL_PASSWORD, MAVEN_GPG_PASSPHRASE in the environment)
mvn -B -f bindings\java\pom.xml -s bindings\java\settings.xml -P release deploy

rem LuaRocks: the rock is made from the git tag v0.2.0, so push the tag first
luarocks upload luarocks\raven-gui-0.2.0-1.rockspec --api-key=<key>
```

A version can be published only once on each registry: to fix a published
package, publish the next version.

Maven Central also takes only signed files: a gpg key is needed, once:

```bat
winget install GnuPG.Gpg4win
gpg --full-generate-key
gpg --keyserver keys.openpgp.org --send-keys <the id of the key>
```

## The others

- **C++**: one header (`cpp/raven.hpp`) over the C library: use it from a
  release of Raven (`find_package(raven)` then include `raven.hpp`).
- **Go**: Go has no registry; modules are fetched from git. The module of
  `bindings/go` is named `raven` (and the projects of the designer
  `import "raven/raven"`), so it is used through a `replace` directive, as
  the designer does: `replace raven => <Raven>/bindings/go`.
- **Zig**: the package manager of Zig fetches whole repositories; use
  `zig/raven.zig` from a release, or `@cImport(@cInclude("Raven.h"))`.
- **Odin**: Odin has no registry and no package manager, by the choice of its
  authors: a package is a folder. Copy `odin/raven/` into the project, or
  point a collection at `bindings/odin` (`-collection:ext=<Raven>/bindings/odin`).
