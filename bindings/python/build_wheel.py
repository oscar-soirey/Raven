"""
Builds the pip package of Raven (a wheel) for one platform.

    python build_wheel.py --prefix <Raven package> [--plat <tag>] [--out dist]

<Raven package> is a shared build of Raven, installed (cmake --install) or
unzipped from a release: the folder that holds lib/ (or bin/ on Windows).
The wheel holds:

    raven/__init__.py        bindings/python/raven.py
    raven/<library>          raven.dll, libraven.so or libraven.dylib

`import raven` then works with nothing else installed: raven.py looks for the
library beside itself first.

--plat is the platform tag of the wheel; by default it is read from the
library itself, so the wheels of every platform can be made on any machine:
    raven.dll        win_amd64
    libraven.dylib   macosx_11_0_universal2 (universal) or _arm64 / _x86_64
    libraven.so      manylinux_2_28_x86_64 (the library must then have been
                     built on glibc 2.28 or older to run everywhere)

Only the Python standard library is used. The version is the one of the
CMake project (CMakeLists.txt at the root of Raven).
"""

import argparse
import base64
import glob
import hashlib
import io
import os
import platform
import re
import stat
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))

NAME = "raven-gui"          # the name on PyPI; `import raven` in Python
DIST = "raven_gui"          # the same, as wheel file names spell it
SUMMARY = "Raven: a lightweight, multithreaded GUI library for editors and tools"
HOMEPAGE = "https://github.com/oscar-soirey/Raven"


def version():
    with open(os.path.join(ROOT, "CMakeLists.txt"), encoding="utf-8") as f:
        match = re.search(r"project\(raven\s+VERSION\s+([0-9.]+)", f.read())
    if not match:
        sys.exit("build_wheel: the version was not found in CMakeLists.txt")
    return match.group(1)


def find(prefix, names, folders):
    for folder in folders:
        for name in names:
            path = os.path.join(prefix, folder, name)
            if os.path.isfile(path) and not os.path.islink(path):
                return path
    for folder in folders:          # libraven.so may only exist as links to libraven.so.0.1.0
        for name in names:
            path = os.path.join(prefix, folder, name)
            if os.path.exists(path):
                return os.path.realpath(path)
    for folder in folders:          # links lost when a package was unzipped on Windows
        for name in names:
            stem, ext = os.path.splitext(name)
            pattern = os.path.join(prefix, folder, name + ".*" if ext == ".so" else stem + ".*" + ext)
            matches = sorted(p for p in glob.glob(pattern) if os.path.isfile(p))
            if matches:
                return matches[-1]
    return None


def library_format(library):
    """'pe', 'elf' or 'macho', and the platform tag, read from the file itself."""
    with open(library, "rb") as f:
        head = f.read(64)
    if head[:2] == b"MZ":
        return "pe", "win_amd64"
    if head[:4] == b"\x7fELF":
        machine = int.from_bytes(head[18:20], "little")
        return "elf", "manylinux_2_28_" + {0x3E: "x86_64", 0xB7: "aarch64"}.get(machine, platform.machine() or "x86_64")
    magic = int.from_bytes(head[:4], "big")
    if magic in (0xCAFEBABE, 0xCAFEBABF):          # universal (fat) Mach-O
        return "macho", "macosx_11_0_universal2"
    if int.from_bytes(head[:4], "little") == 0xFEEDFACF:
        cputype = int.from_bytes(head[4:8], "little")
        return "macho", "macosx_11_0_arm64" if cputype == 0x0100000C else "macosx_10_15_x86_64"
    sys.exit("build_wheel: %s is not a library this script knows" % library)


def metadata(ver):
    readme = ""
    readme_path = os.path.join(HERE, "README.md")
    if os.path.isfile(readme_path):
        with open(readme_path, encoding="utf-8") as f:
            readme = f.read()
    lines = [
        "Metadata-Version: 2.1",
        "Name: " + NAME,
        "Version: " + ver,
        "Summary: " + SUMMARY,
        "Home-page: " + HOMEPAGE,
        "Author: Oscar Soirey",
        "License: Apache-2.0",
        "Project-URL: Source, " + HOMEPAGE,
        "Project-URL: Documentation, " + HOMEPAGE + "#readme",
        "Keywords: gui,ui,vulkan,editor,widgets",
        "Classifier: License :: OSI Approved :: Apache Software License",
        "Classifier: Programming Language :: Python :: 3",
        "Classifier: Programming Language :: C",
        "Classifier: Operating System :: Microsoft :: Windows",
        "Classifier: Operating System :: POSIX :: Linux",
        "Classifier: Operating System :: MacOS",
        "Classifier: Topic :: Software Development :: User Interfaces",
        "Requires-Python: >=3.7",
        "Description-Content-Type: text/markdown",
    ]
    return "\n".join(lines) + "\n\n" + readme


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--prefix", required=True, help="a shared build of Raven, installed")
    parser.add_argument("--plat", help="platform tag of the wheel (guessed by default)")
    parser.add_argument("--out", default=os.path.join(HERE, "dist"), help="output folder")
    args = parser.parse_args()

    prefix = os.path.abspath(args.prefix)
    library = find(prefix, ["raven.dll", "libraven.dll", "libraven.dylib", "libraven.so"], ["bin", "lib", "lib64", "."])
    if not library:
        sys.exit("build_wheel: no shared library of Raven in %s (build Raven with -DRAVEN_BUILD_SHARED=ON)" % prefix)

    kind, guessed = library_format(library)
    library_name = {"pe": "raven.dll", "elf": "libraven.so", "macho": "libraven.dylib"}[kind]

    ver = version()
    plat = args.plat or guessed
    tag = "py3-none-" + plat
    files = []      # (path in the wheel, bytes, executable)

    def add_file(arcname, path, executable=False):
        with open(path, "rb") as f:
            files.append((arcname, f.read(), executable))

    add_file("raven/__init__.py", os.path.join(HERE, "raven.py"))
    add_file("raven/" + library_name, library)
    for license_name in ("LICENSE", "NOTICE"):
        path = os.path.join(ROOT, license_name)
        if os.path.isfile(path):
            add_file("%s-%s.dist-info/%s" % (DIST, ver, license_name), path)

    info = "%s-%s.dist-info" % (DIST, ver)
    files.append((info + "/METADATA", metadata(ver).encode("utf-8"), False))
    files.append((info + "/WHEEL", ("Wheel-Version: 1.0\nGenerator: raven build_wheel.py\nRoot-Is-Purelib: false\nTag: %s\n" % tag).encode(), False))
    files.append((info + "/top_level.txt", b"raven\n", False))

    record = io.StringIO()
    for arcname, data, _ in files:
        digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode()
        record.write("%s,sha256=%s,%d\n" % (arcname, digest, len(data)))
    record.write(info + "/RECORD,,\n")
    files.append((info + "/RECORD", record.getvalue().encode(), False))

    os.makedirs(args.out, exist_ok=True)
    wheel = os.path.join(args.out, "%s-%s-%s.whl" % (DIST, ver, tag))
    with zipfile.ZipFile(wheel, "w", zipfile.ZIP_DEFLATED) as z:
        for arcname, data, executable in files:
            entry = zipfile.ZipInfo(arcname, date_time=(2020, 1, 1, 0, 0, 0))
            mode = 0o755 if executable or arcname.endswith((".so", ".dylib")) else 0o644
            entry.external_attr = (stat.S_IFREG | mode) << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(entry, data)
    print(wheel)


if __name__ == "__main__":
    main()
