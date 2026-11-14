#!/bin/sh
# Turns showcase.xml and photo.png into C arrays (gen/*.h) for showcase.c.
# Called by mobile/ios/build.sh and mobile/android/build.sh.
set -e
cd "$(dirname "$0")"
mkdir -p gen
xxd -i showcase.xml > gen/showcase_xml.h
xxd -i photo.png > gen/photo_png.h
