#!/bin/sh
# Runs `conan create` on the recipe of Raven with the sources of this working
# tree (not of a release): a copy of the recipe whose conandata.yml names
# `git archive HEAD` (file://) and its sha256. Commit first: what is not
# committed is not in the archive. test_package builds and runs a program.
#
#   packaging/conan/test-local.sh [work folder] [conan create options...]
#   packaging/conan/test-local.sh build/conan-test -o "&:shared=True" -o "&:tools=True"
#
# CONAN_HOME is a folder of the work folder (a profile is detected there), so
# the Conan cache of the user is not touched.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=$(cd "$HERE/../.." && pwd)
WORK=${1:-"$RAVEN/build/conan-test"}
if [ $# -gt 0 ]; then shift; fi

mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
rm -rf "$WORK/recipe"
mkdir -p "$WORK/recipe"
cp -R "$HERE/conanfile.py" "$HERE/test_package" "$WORK/recipe/"

VERSION=$(sed -n 's/^    version = "\(.*\)"$/\1/p' "$HERE/conanfile.py")
git -C "$RAVEN" archive --format=tar.gz --prefix="Raven-$VERSION/" -o "$WORK/raven-src.tar.gz" HEAD
SHA256=$(python3 -c 'import hashlib,sys; print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$WORK/raven-src.tar.gz")
cat > "$WORK/recipe/conandata.yml" <<EOF
sources:
  "$VERSION":
    url: "file://$WORK/raven-src.tar.gz"
    sha256: "$SHA256"
EOF

export CONAN_HOME="$WORK/conan-home"
conan profile detect --exist-ok
conan create "$WORK/recipe" --build=missing "$@"
