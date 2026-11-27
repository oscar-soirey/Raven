#!/bin/sh
# Builds the vcpkg port of Raven from this working tree (not from GitHub), then
# a program against it with find_package(raven) and with pkg-config.
#
#   packaging/vcpkg/test-local.sh <vcpkg root> [triplet] [work folder]
#
# The port downloads the archive of a release tag; here an overlay copy of it
# takes `git archive HEAD` instead (vcpkg_extract_source_archive), so what is
# tested is the commit checked out, before any tag exists. Commit first:
# what is not committed is not in the archive.
set -eu

VCPKG_ROOT=$(cd "${1:?usage: test-local.sh <vcpkg root> [triplet] [work folder]}" && pwd)
TRIPLET=${2:-}
HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=$(cd "$HERE/../.." && pwd)
WORK=${3:-"$RAVEN/build/vcpkg-test"}
FEATURES=${RAVEN_VCPKG_FEATURES:-tools}

rm -rf "$WORK"
mkdir -p "$WORK/overlay/raven-gui"

# the sources: the archive of HEAD
git -C "$RAVEN" archive --format=tar.gz --prefix=Raven/ -o "$WORK/raven-src.tar.gz" HEAD

# the overlay port: the same files, the sources taken from that archive
cp "$HERE/ports/raven-gui/"* "$WORK/overlay/raven-gui/"
python3 - "$WORK/overlay/raven-gui/portfile.cmake" "$WORK/raven-src.tar.gz" <<'EOF'
import re, sys
path, archive = sys.argv[1], sys.argv[2]
text = open(path).read()
text, n = re.subn(r"vcpkg_from_github\(.*?\n\)", 'vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "%s")' % archive, text, count=1, flags=re.S)
if n != 1:
    sys.exit("test-local.sh: no vcpkg_from_github(...) in the portfile")
open(path, "w").write(text)
EOF

TRIPLET_ARG=""
if [ -n "$TRIPLET" ]; then TRIPLET_ARG="--triplet=$TRIPLET"; fi
"$VCPKG_ROOT/vcpkg" install "raven-gui[$FEATURES]" $TRIPLET_ARG \
    --overlay-ports="$WORK/overlay" \
    --x-install-root="$WORK/installed" \
    --x-buildtrees-root="$WORK/buildtrees" \
    --x-packages-root="$WORK/packages" \
    --no-binarycaching

# the installed triplet: the one given, or the folder of installed/ that holds raven.pc
if [ -n "$TRIPLET" ]; then
    INSTALLED="$WORK/installed/$TRIPLET"
else
    INSTALLED=$(dirname "$(dirname "$(dirname "$(find "$WORK/installed" -path '*/debug' -prune -o -name raven.pc -print | head -n 1)")")")
fi
echo "== installed: $INSTALLED"

# 1. find_package(raven) with the toolchain of vcpkg
cmake -S "$RAVEN/tests/package" -B "$WORK/consumer-cmake" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_INSTALLED_DIR="$WORK/installed" ${TRIPLET:+-DVCPKG_TARGET_TRIPLET=$TRIPLET} \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/consumer-cmake" --config Release
PROGRAM=$(find "$WORK/consumer-cmake" -name 'raven_consumer' -o -name 'raven_consumer.exe' | head -n 1)
"$PROGRAM"

# 2. pkg-config
if command -v pkg-config >/dev/null 2>&1; then
    export PKG_CONFIG_PATH="$INSTALLED/lib/pkgconfig"
    PCFILEDIR=$(pkg-config --variable=pcfiledir raven)
    case "$PCFILEDIR" in
        "$INSTALLED"/*) ;;
        *) echo "test-local.sh: pkg-config found another raven.pc: $PCFILEDIR"; exit 1 ;;
    esac
    FLAGS=$(pkg-config --cflags --libs --static raven)
    echo "== pkg-config --cflags --libs --static raven: $FLAGS"
    # (a shared triplet: the library is found beside the program at run time)
    cc "$RAVEN/tests/package/consumer.c" -o "$WORK/consumer-pkgconfig" $FLAGS -Wl,-rpath,"$INSTALLED/lib"
    "$WORK/consumer-pkgconfig"
fi

# 3. the tools, when built
for tool in raven_designer raven_theme_editor; do
    if [ -x "$INSTALLED/tools/raven-gui/$tool" ]; then
        RAVEN_HEADLESS=1 RAVEN_FRAMES=2 "$INSTALLED/tools/raven-gui/$tool"
        echo "== $tool runs headless"
    fi
done
echo "== the vcpkg port of $(git -C "$RAVEN" rev-parse --short HEAD) works"
