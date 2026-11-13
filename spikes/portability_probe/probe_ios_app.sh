#!/bin/sh
# Builds an example as an iOS application for the simulator (UIKit platform, Sokol/Metal backend), installs it
# in the booted simulator and starts it. Run it on a Mac with Xcode. Written without an iOS SDK at hand: expect a
# first fix or two.
#
#   ./probe_ios_app.sh [example: hello|glass] [build dir]
#
# Boot a simulator first (open Simulator.app, or: xcrun simctl boot "iPhone 17"). Touch the screen with the
# mouse; with a hardware keyboard connected (Simulator: I/O > Keyboard > Connect Hardware Keyboard) type into
# the text field of hello. The screenshot is written next to the build.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT="$HERE/../.."
EXAMPLE=${1:-hello}
BUILD=${2:-/tmp/raven-ios-app}
ARCH=$(uname -m); [ "$ARCH" = x86_64 ] || ARCH=arm64

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=$ARCH \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
  -DRAVEN_BACKEND_VULKAN=OFF -DRAVEN_BACKEND_SOKOL=ON -DRAVEN_BUILD_WEB=OFF \
  -DRAVEN_BUILD_EXAMPLES=ON -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_TESTS=OFF
cmake --build "$BUILD" --target raven_$EXAMPLE --parallel

APP=$(find "$BUILD" -name "raven_$EXAMPLE.app" -type d | head -1)
[ -n "$APP" ] || { echo "no raven_$EXAMPLE.app in $BUILD"; exit 1; }
codesign --force --sign - "$APP"

echo "== simulator"
xcrun simctl list devices booted
if ! xcrun simctl list devices booted | grep -q Booted; then
  echo "no simulator is booted: xcrun simctl boot \"iPhone 17\" (and open -a Simulator), then run this again"
  exit 1
fi
# waits until the booted simulator is ready to install into
xcrun simctl bootstatus booted
echo "== install"
xcrun simctl install booted "$APP"
echo "== launch"
xcrun simctl launch booted org.raven.example.$EXAMPLE
sleep 3
xcrun simctl io booted screenshot "$BUILD/ios_$EXAMPLE.png"
echo "started: $APP"
echo "screenshot: $BUILD/ios_$EXAMPLE.png"
