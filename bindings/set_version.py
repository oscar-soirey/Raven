"""
Sets the version of every binding package from the one of Raven.

    python bindings/set_version.py            # the version of CMakeLists.txt
    python bindings/set_version.py 0.2.0      # also writes it in CMakeLists.txt

Updates: CMakeLists.txt (if a version is given), the fallback version of
src/core/base.h, the version macros of include/raven/raven_core.h, the Rust crate
(rust/Cargo.toml), the Bun package (bun/package.json), the Node.js package
(node/package.json), the NuGet package
(csharp/Raven.Gui.csproj), the Maven package (java/pom.xml, its README) and
the LuaRocks rockspec (luajit/*.rockspec,
renamed to the new version). The pip package reads CMakeLists.txt itself.
"""

import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)


def read(path):
    with open(path, encoding="utf-8", newline="") as f:
        return f.read()


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def replace(path, pattern, value, count=1):
    text = read(path)
    new, n = re.subn(pattern, value, text, count=count, flags=re.M)
    if n == 0:
        sys.exit("set_version: nothing to change in %s (%s)" % (path, pattern))
    if new != text:
        write(path, new)
    print("  " + os.path.relpath(path, ROOT))


def main():
    cmake = os.path.join(ROOT, "CMakeLists.txt")
    if len(sys.argv) > 1:
        version = sys.argv[1].lstrip("v")
        if not re.fullmatch(r"\d+\.\d+\.\d+", version):
            sys.exit("set_version: the version must be X.Y.Z")
        replace(cmake, r"(project\(raven\s+VERSION\s+)[0-9.]+", r"\g<1>" + version)
    else:
        match = re.search(r"project\(raven\s+VERSION\s+([0-9.]+)", read(cmake))
        if not match:
            sys.exit("set_version: no version in CMakeLists.txt")
        version = match.group(1)
    print("version " + version)

    replace(os.path.join(ROOT, "src", "core", "base.h"), r'^(#define RVN_VERSION_STRING ")[^"]*(")', r"\g<1>%s\g<2>" % version)
    major, minor, patch = version.split(".")
    core = os.path.join(ROOT, "include", "raven", "raven_core.h")
    replace(core, r"^(#define RAVEN_VERSION_MAJOR )\d+", r"\g<1>" + major)
    replace(core, r"^(#define RAVEN_VERSION_MINOR )\d+", r"\g<1>" + minor)
    replace(core, r"^(#define RAVEN_VERSION_PATCH )\d+", r"\g<1>" + patch)
    replace(core, r'^(#define RAVEN_API_VERSION ")[^"]*(")', r"\g<1>%s.%s\g<2>" % (major, minor))
    replace(os.path.join(HERE, "rust", "Cargo.toml"), r'^(version\s*=\s*")[^"]*(")', r"\g<1>%s\g<2>" % version)
    replace(os.path.join(HERE, "bun", "package.json"), r'^(\s*"version":\s*")[^"]*(")', r"\g<1>%s\g<2>" % version)
    replace(os.path.join(HERE, "node", "package.json"), r'^(\s*"version":\s*")[^"]*(")', r"\g<1>%s\g<2>" % version)
    replace(os.path.join(HERE, "csharp", "Raven.Gui.csproj"), r"(<Version>)[^<]*(</Version>)", r"\g<1>%s\g<2>" % version)
    replace(os.path.join(HERE, "java", "pom.xml"), r"(<artifactId>raven-gui</artifactId>\s*<version>)[^<]*(</version>)", r"\g<1>%s\g<2>" % version)
    for readme in (os.path.join(HERE, "java", "README.md"), os.path.join(HERE, "README.md")):
        replace(readme, r"(<version>|raven-gui:)\d+\.\d+\.\d+", r"\g<1>%s" % version, count=0)

    rockspecs = glob.glob(os.path.join(HERE, "luajit", "raven-gui-*.rockspec"))
    if len(rockspecs) != 1:
        sys.exit("set_version: expected one luajit/raven-gui-*.rockspec, found %d" % len(rockspecs))
    old = rockspecs[0]
    text = read(old)
    text = re.sub(r'^version = "[^"]*"', 'version = "%s-1"' % version, text, flags=re.M)
    text = re.sub(r'^(\s*tag = )"[^"]*"', r'\g<1>"v%s"' % version, text, flags=re.M)
    text = re.sub(r"raven-gui-[0-9.]+-\d+\.rockspec", "raven-gui-%s-1.rockspec" % version, text)
    new = os.path.join(HERE, "luajit", "raven-gui-%s-1.rockspec" % version)
    write(new, text)
    if os.path.abspath(new) != os.path.abspath(old):
        os.remove(old)
    print("  " + os.path.relpath(new, ROOT))


if __name__ == "__main__":
    main()
