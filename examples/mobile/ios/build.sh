#!/bin/sh
# Raven on iOS (experimental): builds libraven (CMake + Ninja), then the showcase
# (../shared) as Hello.app by hand (clang + Info.plist + signature), installs and
# launches it. Raven draws with Metal: nothing else is needed (no MoltenVK).
# See ../README.md.
#   ./build.sh            build for the iOS Simulator
#   ./build.sh run        build, install, launch on the Simulator (console attached)
#   ./build.sh device     build for an iPhone or iPad (arm64): signed when SIGN_IDENTITY
#                         and PROFILE are given, otherwise only built (see ../README.md)
#   ./build.sh device run build, sign, install and launch on DEVICE (untested)
# UDID=<simulator> picks the simulator; by default the booted one.
# XCFRAMEWORK=1 links the slice of Raven.xcframework (tools/mobile/build-xcframework.sh,
# built or brought up to date first) instead of the CMake build; XCFRAMEWORK=<path>
# uses that xcframework.
# RAVEN_CMAKE_ARGS is added to the CMake configuration of Raven (CI: -DRAVEN_WARNINGS_AS_ERRORS=ON).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=${RAVEN:-$(cd "$HERE/../../.." && pwd)}
UDID=${UDID:-booted}
APP=$HERE/build/Hello.app
export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH

TARGET=simulator
if [ "${1:-}" = device ]; then TARGET=device; shift; fi
if [ $TARGET = device ]; then
	SDK_NAME=iphoneos
	TRIPLE=arm64-apple-ios17.0
	SLICE=ios-arm64
	PLATFORM=iPhoneOS
	BUILD=$RAVEN/build-ios-device
else
	SDK_NAME=iphonesimulator
	TRIPLE=arm64-apple-ios17.0-simulator
	SLICE=ios-arm64_x86_64-simulator
	PLATFORM=iPhoneSimulator
	BUILD=$RAVEN/build-ios-sim
fi

# libraven.a and its headers: the CMake build of this target, or the xcframework's slice
if [ -n "${XCFRAMEWORK:-}" ]; then
	if [ "$XCFRAMEWORK" = 1 ]; then
		"$RAVEN/tools/mobile/build-xcframework.sh" "$RAVEN/build-xcframework"
		XCFRAMEWORK=$RAVEN/build-xcframework/Raven.xcframework
	fi
	LIBRAVEN=$XCFRAMEWORK/$SLICE/libraven.a
	INCLUDE=$XCFRAMEWORK/$SLICE/Headers
else
	if [ ! -f "$BUILD/build.ninja" ]; then
		cmake -S "$RAVEN" -B "$BUILD" -G Ninja -DCMAKE_SYSTEM_NAME=iOS \
			-DCMAKE_OSX_SYSROOT=$SDK_NAME -DCMAKE_OSX_ARCHITECTURES=arm64 \
			-DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 -DCMAKE_BUILD_TYPE=Debug -DRAVEN_BACKEND_VULKAN=OFF \
			-DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_WEB=OFF \
			${RAVEN_CMAKE_ARGS:-}
	fi
	cmake --build "$BUILD"
	LIBRAVEN=$BUILD/libraven.a
	INCLUDE=$RAVEN/include
fi

rm -rf "$APP"; mkdir -p "$APP"
SDK=$(xcrun --sdk $SDK_NAME --show-sdk-path)
CC="xcrun --sdk $SDK_NAME clang -target $TRIPLE -isysroot $SDK"
SHARED=$HERE/../shared
$CC -g -fobjc-arc -I"$INCLUDE" -I"$SHARED" -c "$HERE/src/main.m" -o "$HERE/build/main.o"
$CC -g -std=c11 -Wall -I"$INCLUDE" -I"$SHARED" -c "$SHARED/showcase.c" -o "$HERE/build/showcase.o"
$CC -o "$APP/Hello" "$HERE/build/main.o" "$HERE/build/showcase.o" "$LIBRAVEN" \
	-framework UIKit -framework QuartzCore -framework Metal -framework CoreFoundation -framework Foundation \
	-framework CoreText -framework CoreGraphics \
	-framework AVFoundation -framework Photos -framework PhotosUI -framework UniformTypeIdentifiers \
	-framework UserNotifications -framework ImageIO -framework CoreLocation
cp "$HERE/Info.plist" "$APP/Info.plist"
plutil -replace CFBundleSupportedPlatforms -json "[\"$PLATFORM\"]" "$APP/Info.plist"
[ -z "${BUNDLE_ID:-}" ] || plutil -replace CFBundleIdentifier -string "$BUNDLE_ID" "$APP/Info.plist"
# the files the app reads (its resources): ../shared/assets + the font of the titles
cp -R "$SHARED/assets/." "$APP/"
mkdir -p "$APP/fonts"
cp "$RAVEN/tools/designer/fonts/BricolageGrotesque-Bold-subset.ttf" "$APP/fonts/"

if [ $TARGET = simulator ]; then
	codesign -s - --force "$APP"
	echo "built $APP"
	if [ "${1:-}" = run ]; then
		[ "$UDID" = booted ] || xcrun simctl boot "$UDID" 2>/dev/null || true
		xcrun simctl install "$UDID" "$APP"
		xcrun simctl launch --console-pty --terminate-running-process "$UDID" dev.raven.ios.hello
	fi
	exit 0
fi

# a device: signed with the developer's identity and provisioning profile (untested here)
if [ -z "${SIGN_IDENTITY:-}" ] || [ -z "${PROFILE:-}" ]; then
	echo "built $APP (arm64, iphoneos), not signed: a device needs SIGN_IDENTITY=<identity of"
	echo "'security find-identity -v -p codesigning'> and PROFILE=<.mobileprovision that allows"
	echo "the bundle id, BUNDLE_ID=<id> to change it>; see ../README.md"
	exit 0
fi
cp "$PROFILE" "$APP/embedded.mobileprovision"
security cms -D -i "$PROFILE" > "$HERE/build/profile.plist"
plutil -extract Entitlements xml1 -o "$HERE/build/entitlements.plist" "$HERE/build/profile.plist"
codesign -s "$SIGN_IDENTITY" --force --entitlements "$HERE/build/entitlements.plist" "$APP"
echo "built and signed $APP"
if [ "${1:-}" = run ]; then
	[ -n "${DEVICE:-}" ] || { echo "DEVICE=<id of 'xcrun devicectl list devices'> is needed to install" >&2; exit 1; }
	xcrun devicectl device install app --device "$DEVICE" "$APP"
	xcrun devicectl device process launch --device "$DEVICE" "$(plutil -extract CFBundleIdentifier raw "$APP/Info.plist")"
fi
