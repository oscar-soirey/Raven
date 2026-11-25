#!/bin/sh
# Smoke test of the showcase APK on the device or emulator adb sees (ANDROID_SERIAL picks
# one; CI: the android-emulator job): installs raven-hello.apk (./build.sh), launches it,
# waits, then checks that the app is still running, in front, and drawing: a screenshot
# with more than 50 colors (a blank or black screen has only a few). Prints the app's log
# when a check fails.
# Usage: ./smoke.sh [apk]
# WAIT=<seconds> after the launch (default 20); SCREENSHOT=<png> (default
# android-smoke.png in the current directory) is kept for a look.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
APK=${1:-$HERE/raven-hello.apk}
WAIT=${WAIT:-20}
SCREENSHOT=${SCREENSHOT:-android-smoke.png}
PKG=dev.raven.hello
RAW=${TMPDIR:-/tmp}/raven-smoke-$$.raw
trap 'rm -f "$RAW"' EXIT

fail() {
	echo "smoke: $*" >&2
	echo "--- logcat (Raven, RavenShowcase, RavenHello, crashes)"
	adb logcat -d -s Raven RavenShowcase RavenHello AndroidRuntime:E DEBUG:* libc:F || true
	exit 1
}

adb wait-for-device
adb install -r "$APK"
adb logcat -c || true
adb shell am start -W -n "$PKG/android.app.NativeActivity"
perl -e "sleep $WAIT"

# still running
PID=$(adb shell pidof "$PKG" | tr -d '\r')
[ -n "$PID" ] || fail "$PKG is not running"
echo "running: pid $PID"
# in front (and so the screenshot shows the app, nothing else)
adb shell dumpsys window | grep mCurrentFocus | grep -q "$PKG" || {
	adb shell dumpsys window | grep mCurrentFocus || true
	fail "$PKG is not in front"
}

# and drawing: the raw frame (width, height, format, [color space], RGBA pixels)
adb exec-out screencap -p > "$SCREENSHOT"
adb exec-out screencap > "$RAW"
python3 - "$RAW" <<'EOF' || fail "the screen is blank"
import struct, sys
b = open(sys.argv[1], "rb").read()
w, h, fmt = struct.unpack("<3I", b[:12])
o = len(b) - w * h * 4
if fmt != 1 or o not in (12, 16):
    sys.exit("unexpected screencap: %dx%d, format %d, %d bytes" % (w, h, fmt, len(b)))
n = len({b[i:i + 3] for i in range(o, len(b), 4 * 97)})
print("screen %dx%d, colors: %d" % (w, h, n))
sys.exit(0 if n > 50 else 1)
EOF
echo "smoke: OK ($SCREENSHOT)"
