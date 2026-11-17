#!/bin/sh
# Raven on Android (experimental): builds Raven (NDK, arm64-v8a, plus its Java text
# view) and the showcase APK (../shared) by hand, without Gradle. See ../README.md.
# Usage: ./build.sh [install]
# Uses the newest NDK, build tools and platform of the SDK unless ANDROID_NDK,
# ANDROID_BUILD_TOOLS or ANDROID_JAR say otherwise.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
RAVEN=${RAVEN:-$(cd "$HERE/../../.." && pwd)}
SDK=${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$HOME/Library/Android/sdk}}
NDK=${ANDROID_NDK:-$(ls -d "$SDK"/ndk/* | tail -1)}
BT=${ANDROID_BUILD_TOOLS:-$(ls -d "$SDK"/build-tools/* | tail -1)}
JAR=${ANDROID_JAR:-$(ls -d "$SDK"/platforms/android-* | sort -t- -k2 -n | tail -1)/android.jar}
API=29
HOST_TAG=$(uname -s | tr '[:upper:]' '[:lower:]')-x86_64
CC="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin/aarch64-linux-android$API-clang"
GLUE=$NDK/sources/android/native_app_glue
OUT=$HERE/build
export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH

# 1. Raven core
cmake -S "$RAVEN" -B "$RAVEN/build-android" -G Ninja \
	-DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
	-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-$API \
	-DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_WEB=OFF \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo >/dev/null
cmake --build "$RAVEN/build-android"

# 2. the app's .so: main.c + the shared showcase + native_app_glue + libraven.a
SHARED=$HERE/../shared
"$SHARED/gen.sh"
rm -rf "$OUT"
mkdir -p "$OUT/apk/lib/arm64-v8a"
"$CC" -std=c11 -O2 -g -fPIC -shared -o "$OUT/apk/lib/arm64-v8a/libravenhello.so" \
	-I"$RAVEN/include" -I"$GLUE" -I"$SHARED" \
	"$HERE/main.c" "$SHARED/showcase.c" "$GLUE/android_native_app_glue.c" \
	"$RAVEN/build-android/libraven.a" \
	-u ANativeActivity_onCreate -Wl,-z,max-page-size=16384 \
	-landroid -llog -lm -ldl

# 3. Raven's Java part (the InputConnection of the on-screen keyboard): javac + d8 -> classes.dex
#    (without it Raven falls back to key events: no suggestions, ASCII only)
JAVA_SRC=$RAVEN/src/platform/android/java
mkdir -p "$OUT/classes"
javac -source 8 -target 8 -bootclasspath "$JAR" -Xlint:-options -encoding UTF-8 -d "$OUT/classes" $(find "$JAVA_SRC" -name '*.java')
"$BT/d8" --min-api $API --lib "$JAR" --output "$OUT/apk" $(find "$OUT/classes" -name '*.class')

# 4. APK: aapt2 link (manifest only), add the .so and classes.dex, zipalign, sign
"$BT/aapt2" link -o "$OUT/base.apk" --manifest "$HERE/AndroidManifest.xml" -I "$JAR"
(cd "$OUT/apk" && zip -q -r -0 "$OUT/base.apk" lib classes.dex)
"$BT/zipalign" -f -P 16 4 "$OUT/base.apk" "$OUT/aligned.apk"
if [ ! -f "$HERE/debug.keystore" ]; then
	keytool -genkeypair -keystore "$HERE/debug.keystore" -storepass android -keypass android \
		-alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US" >/dev/null 2>&1
fi
"$BT/apksigner" sign --ks "$HERE/debug.keystore" --ks-pass pass:android --key-pass pass:android \
	--out "$HERE/raven-hello.apk" "$OUT/aligned.apk"
echo "APK: $HERE/raven-hello.apk"

if [ "${1:-}" = "install" ]; then
	adb install -r "$HERE/raven-hello.apk"
	adb shell am start -n dev.raven.hello/android.app.NativeActivity
fi
