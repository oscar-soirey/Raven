#!/bin/sh
# raven-<version>.aar for Android apps, with libraven.so for several ABIs: one CMake build
# per ABI (its raven_aar target, cmake/RavenAndroid.cmake), then their AARs merged into
# one. See examples/mobile/README.md, "Packaging".
#   tools/mobile/build-android-aar.sh [abi ...]     (default: arm64-v8a x86_64)
# OUT (build-aar), CONFIG (Release), API (29, the minSdk) and ANDROID_NDK (the newest NDK
# of ANDROID_HOME) change the build; the SDK's tools are found as cmake/RavenAndroid.cmake says.
set -eu
RAVEN=$(cd "$(dirname "$0")/../.." && pwd)
SDK=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}
NDK=${ANDROID_NDK:-$(ls -d "$SDK"/ndk/* | sort -t. -k1,1n | tail -1)}
OUT=${OUT:-$RAVEN/build-aar}
CONFIG=${CONFIG:-Release}
API=${API:-29}
[ $# -gt 0 ] || set -- arm64-v8a x86_64
export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)

MERGED=$OUT/aar
rm -rf "$MERGED"
for ABI in "$@"; do
	cmake -S "$RAVEN" -B "$OUT/$ABI" -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
		-DANDROID_ABI="$ABI" -DANDROID_PLATFORM=android-$API -DRAVEN_BUILD_SHARED=ON \
		-DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_WEB=OFF \
		-DCMAKE_BUILD_TYPE="$CONFIG" >/dev/null
	cmake --build "$OUT/$ABI" --target raven_aar
	STAGE=$OUT/$ABI/android/aar
	[ -f "$STAGE/classes.jar" ] || { echo "no AAR for $ABI (see the CMake warnings)" >&2; exit 1; }
	if [ ! -d "$MERGED" ]; then
		cp -R "$STAGE" "$MERGED"
	else
		cp -R "$STAGE/jni/$ABI" "$MERGED/jni/"
		cp -R "$STAGE/prefab/modules/raven/libs/android.$ABI" "$MERGED/prefab/modules/raven/libs/"
	fi
done

VERSION=$(sed -n 's/.*android:versionName="\([^"]*\)".*/\1/p' "$MERGED/AndroidManifest.xml")
AAR=$OUT/raven-$VERSION.aar
rm -f "$AAR"
(cd "$MERGED" && cmake -E tar cf "$AAR" --format=zip AndroidManifest.xml classes.jar proguard.txt R.txt jni prefab)
echo "AAR: $AAR ($*)"
