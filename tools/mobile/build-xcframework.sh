#!/bin/sh
# Raven.xcframework for iOS apps: libraven.a for devices (ios-arm64) and for the
# Simulator (ios-arm64_x86_64-simulator), each with Raven's public headers and a module
# map (module Raven). See examples/mobile/README.md, "Packaging".
#   tools/mobile/build-xcframework.sh [output folder]     (default: build-xcframework)
# CONFIG (Release), MIN_IOS (15.0) and SIM_ARCHS ("arm64;x86_64") change the build.
# Raven draws with Metal (RAVEN_BACKEND_VULKAN=OFF): the app embeds nothing else (no MoltenVK).
# RAVEN_CMAKE_ARGS is added to the CMake configuration of both slices.
# The app links the system frameworks Raven uses (the module map links them for
# `import Raven` / `@import Raven;`): Foundation, CoreFoundation, UIKit, QuartzCore, Metal,
# CoreText, CoreGraphics, and for the services of the device AVFoundation, Photos,
# PhotosUI, UniformTypeIdentifiers, UserNotifications, ImageIO, CoreLocation.
set -eu
RAVEN=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-$RAVEN/build-xcframework}
CONFIG=${CONFIG:-Release}
MIN_IOS=${MIN_IOS:-15.0}
SIM_ARCHS=${SIM_ARCHS:-arm64;x86_64}
export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)

# <sdk> <architectures>: libraven.a in $OUT/<sdk>
build() {
	cmake -S "$RAVEN" -B "$OUT/$1" -G Ninja -DCMAKE_SYSTEM_NAME=iOS \
		-DCMAKE_OSX_SYSROOT="$1" -DCMAKE_OSX_ARCHITECTURES="$2" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_IOS" -DCMAKE_BUILD_TYPE="$CONFIG" -DRAVEN_BACKEND_VULKAN=OFF \
		-DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_WEB=OFF \
		${RAVEN_CMAKE_ARGS:-} >/dev/null
	cmake --build "$OUT/$1"
}
build iphoneos arm64
build iphonesimulator "$SIM_ARCHS"

# the headers of both slices: Raven.h, raven/, module.modulemap
rm -rf "$OUT/headers"
mkdir -p "$OUT/headers"
cp "$RAVEN/include/Raven.h" "$OUT/headers/"
cp -R "$RAVEN/include/raven" "$OUT/headers/"
cp "$RAVEN/tools/mobile/module.modulemap" "$OUT/headers/"

rm -rf "$OUT/Raven.xcframework"
xcodebuild -create-xcframework \
	-library "$OUT/iphoneos/libraven.a" -headers "$OUT/headers" \
	-library "$OUT/iphonesimulator/libraven.a" -headers "$OUT/headers" \
	-output "$OUT/Raven.xcframework"
echo "xcframework: $OUT/Raven.xcframework"
