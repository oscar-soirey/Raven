#!/bin/sh
# Phase 0b, iOS: build the Raven core for the iOS simulator (null platform, no GPU backend) and run the headless
# tests inside the simulator. Run it on a Mac with Xcode. Written without a Mac: expect a first fix or two.
#
#   ./probe_ios.sh [build dir]
#
# Boot a simulator first (open Simulator.app, or: xcrun simctl list devices available, then
# xcrun simctl boot "<name of one>", for example "iPhone 17"); the tests are started with
# `xcrun simctl spawn booted`, as plain executables (no app bundle needed).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT="$HERE/../.."
BUILD=${1:-/tmp/raven-ios-sim}
ARCH=$(uname -m); [ "$ARCH" = x86_64 ] || ARCH=arm64
TESTS="test_layout test_widgets test_xml test_memory test_animation test_text_editor test_shell test_dialogs test_themes test_markdown test_tools test_locale test_threads test_editor_features test_dock_tearing"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=$ARCH \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DCMAKE_MACOSX_BUNDLE=OFF \
  -DRAVEN_PLATFORM=null -DRAVEN_BACKEND_VULKAN=OFF -DRAVEN_BUILD_WEB=OFF \
  -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_TESTS=ON
cmake --build "$BUILD" --target raven --parallel
echo "== the library builds for the iOS simulator"

# only the plain tests are built (a cross build cannot run what it builds)
for t in $TESTS; do cmake --build "$BUILD" --target $t --parallel >/dev/null; done
BIN="$BUILD/bin"; [ -d "$BIN" ] || BIN="$BUILD/tests"
FAIL=0
for t in $TESTS; do
  out=$(xcrun simctl spawn booted "$BIN/$t" 2>&1) && code=0 || code=$?
  printf '%-22s exit=%s  %s\n' "$t" "$code" "$(echo "$out" | tail -1 | cut -c1-80)"
  [ "$code" = 0 ] || FAIL=1
done
exit $FAIL
