#!/bin/sh
# Raven on iOS (experimental): builds libraven for the iOS Simulator (CMake + Ninja), then
# the showcase (../shared) as Hello.app by hand (clang + Info.plist + embedded
# MoltenVK.framework + ad-hoc signature), installs and launches it. See ../README.md.
#   ./build.sh            build only
#   ./build.sh run        build, install, launch (console attached)
# UDID=<simulator> picks the simulator; by default the booted one.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=${RAVEN:-$(cd "$HERE/../../.." && pwd)}
BUILD=$RAVEN/build-ios-sim
UDID=${UDID:-booted}
MVK=$HERE/MoltenVK/MoltenVK/dynamic/MoltenVK.xcframework/ios-arm64_x86_64-simulator/MoltenVK.framework
APP=$HERE/build/Hello.app
export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH

if [ ! -d "$MVK" ]; then
	echo "MoltenVK for the simulator is missing: extract MoltenVK-all.tar from" >&2
	echo "https://github.com/KhronosGroup/MoltenVK/releases into $HERE/MoltenVK (see ../README.md)" >&2
	exit 1
fi

if [ ! -f "$BUILD/build.ninja" ]; then
	cmake -S "$RAVEN" -B "$BUILD" -G Ninja -DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 -DCMAKE_BUILD_TYPE=Debug \
		-DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_WEB=OFF
fi
cmake --build "$BUILD"

rm -rf "$APP"; mkdir -p "$APP/Frameworks"
SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
CC="xcrun --sdk iphonesimulator clang -target arm64-apple-ios17.0-simulator -isysroot $SDK"
SHARED=$HERE/../shared
"$SHARED/gen.sh"
$CC -g -fobjc-arc -I"$RAVEN/include" -I"$SHARED" -c "$HERE/src/main.m" -o "$HERE/build/main.o"
$CC -g -std=c11 -Wall -I"$RAVEN/include" -I"$SHARED" -c "$SHARED/showcase.c" -o "$HERE/build/showcase.o"
$CC -o "$APP/Hello" "$HERE/build/main.o" "$HERE/build/showcase.o" "$BUILD/libraven.a" \
	-framework UIKit -framework QuartzCore -framework Metal -framework CoreFoundation -framework Foundation \
	-Wl,-rpath,@executable_path/Frameworks
cp "$HERE/Info.plist" "$APP/Info.plist"
cp -R "$MVK" "$APP/Frameworks/"
codesign -s - --force "$APP/Frameworks/MoltenVK.framework"
codesign -s - --force "$APP"
echo "built $APP"

if [ "$1" = "run" ]; then
	[ "$UDID" = booted ] || xcrun simctl boot "$UDID" 2>/dev/null || true
	xcrun simctl install "$UDID" "$APP"
	xcrun simctl launch --console-pty --terminate-running-process "$UDID" dev.raven.ios.hello
fi
