"""
Writes the checksums of the archive of a release tag into the Homebrew
formula (sha256), the vcpkg port (SHA512) and the Conan recipe (sha256).

    python packaging/update_hashes.py                 # downloads the archive of v<version of CMakeLists.txt>
    python packaging/update_hashes.py --archive x.tar.gz   # an archive already downloaded

Run it once the tag v<version> is pushed (GitHub makes the archive from the
tag): bindings/set_version.py sets a new version with zeros in their place.
The archive is the one GitHub serves for the tag,
https://github.com/oscar-soirey/Raven/archive/refs/tags/v<version>.tar.gz,
the same file Homebrew, vcpkg (vcpkg_from_github) and Conan download.
"""

import argparse
import hashlib
import os
import re
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REPOSITORY = "oscar-soirey/Raven"


def read(path):
    with open(path, encoding="utf-8", newline="") as f:
        return f.read()


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def replace(path, pattern, value):
    text = read(path)
    new, n = re.subn(pattern, value, text, count=1, flags=re.M)
    if n == 0:
        sys.exit("update_hashes: nothing to change in %s (%s)" % (path, pattern))
    if new != text:
        write(path, new)
    print("  " + os.path.relpath(path, ROOT))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--archive", help="the archive of the tag, already downloaded")
    args = parser.parse_args()

    match = re.search(r"project\(raven\s+VERSION\s+([0-9.]+)", read(os.path.join(ROOT, "CMakeLists.txt")))
    if not match:
        sys.exit("update_hashes: no version in CMakeLists.txt")
    version = match.group(1)

    if args.archive:
        with open(args.archive, "rb") as f:
            data = f.read()
        print("archive " + args.archive)
    else:
        url = "https://github.com/%s/archive/refs/tags/v%s.tar.gz" % (REPOSITORY, version)
        print("downloading " + url)
        with urllib.request.urlopen(url) as response:
            data = response.read()
    sha256 = hashlib.sha256(data).hexdigest()
    sha512 = hashlib.sha512(data).hexdigest()
    print("version %s\n  sha256 %s\n  sha512 %s" % (version, sha256, sha512))

    formula = os.path.join(HERE, "homebrew", "raven-gui.rb")
    if not re.search(r'/v%s\.tar\.gz"' % re.escape(version), read(formula)):
        sys.exit("update_hashes: the formula is not of version %s: run bindings/set_version.py first" % version)
    replace(formula, r'^(  sha256 ")[0-9a-f]*(")', r"\g<1>%s\g<2>" % sha256)

    port = os.path.join(HERE, "vcpkg", "ports", "raven-gui")
    if not re.search(r'^  "version": "%s"' % re.escape(version), read(os.path.join(port, "vcpkg.json")), flags=re.M):
        sys.exit("update_hashes: the vcpkg port is not of version %s: run bindings/set_version.py first" % version)
    replace(os.path.join(port, "portfile.cmake"), r"^(    SHA512 )[0-9a-f]+$", r"\g<1>%s" % sha512)

    conandata = os.path.join(HERE, "conan", "conandata.yml")
    if not re.search(r'^  "%s":' % re.escape(version), read(conandata), flags=re.M):
        sys.exit("update_hashes: conandata.yml has no version %s: run bindings/set_version.py first" % version)
    replace(conandata, r'^(    sha256: ")[0-9a-f]*(")', r"\g<1>%s\g<2>" % sha256)


if __name__ == "__main__":
    main()
