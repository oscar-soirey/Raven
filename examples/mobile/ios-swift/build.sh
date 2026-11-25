#!/bin/sh
# Raven on iOS from Swift (experimental): a UIKit app in Swift (src/App.swift)
# over Raven.xcframework and the Swift binding (bindings/swift), built by hand
# with swiftc (no Xcode project) as HelloSwift.app. See ../README.md, "Swift".
#   ./build.sh            build for the iOS Simulator
#   ./build.sh run        build, install, launch on the Simulator (console attached)
# UDID=<simulator> picks the simulator; by default the booted one.
# XCFRAMEWORK=<path> uses that Raven.xcframework; by default
# tools/mobile/build-xcframework.sh builds (or brings up to date) build-xcframework/.
#
# How the binding goes with the xcframework: the xcframework's module is `Raven`
# (the C API), and bindings/swift/Sources/Raven/Raven.swift is compiled as the
# Swift part of that same module (-module-name Raven -import-underlying-module),
# so that the app's `import Raven` gives both. The module map of the xcframework
# links libraven and the system frameworks.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=${RAVEN:-$(cd "$HERE/../../.." && pwd)}
UDID=${UDID:-booted}
BUILD=$HERE/build
APP=$BUILD/HelloSwift.app
export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH

if [ -z "${XCFRAMEWORK:-}" ]; then
	"$RAVEN/tools/mobile/build-xcframework.sh" "$RAVEN/build-xcframework"
	XCFRAMEWORK=$RAVEN/build-xcframework/Raven.xcframework
fi
SLICE=$XCFRAMEWORK/ios-arm64_x86_64-simulator
SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)
SWIFTC="xcrun --sdk iphonesimulator swiftc -target arm64-apple-ios17.0-simulator -sdk $SDK -swift-version 5 -g"

rm -rf "$APP"; mkdir -p "$APP" "$BUILD/modules"
# the binding: the Swift part of the module Raven (Raven.swiftmodule + Raven.o)
$SWIFTC -parse-as-library -module-name Raven -import-underlying-module -I "$SLICE/Headers" \
	-emit-module -emit-module-path "$BUILD/modules/Raven.swiftmodule" \
	-c "$RAVEN/bindings/swift/Sources/Raven/Raven.swift" -o "$BUILD/Raven.o"
# the app, linked with the binding and libraven.a
$SWIFTC -parse-as-library -module-name HelloSwift -I "$BUILD/modules" -I "$SLICE/Headers" \
	-import-objc-header "$HERE/src/Bridge.h" \
	"$HERE/src/App.swift" "$BUILD/Raven.o" -L "$SLICE" -lraven -o "$APP/HelloSwift"
cp "$HERE/Info.plist" "$APP/Info.plist"
codesign -s - --force "$APP"
echo "built $APP"

if [ "${1:-}" = run ]; then
	[ "$UDID" = booted ] || xcrun simctl boot "$UDID" 2>/dev/null || true
	xcrun simctl install "$UDID" "$APP"
	xcrun simctl launch --console-pty --terminate-running-process "$UDID" dev.raven.ios.swift
fi
