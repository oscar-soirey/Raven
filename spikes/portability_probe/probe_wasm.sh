#!/bin/sh
# Phase 0b: build the Raven core for WebAssembly (null platform, no GPU backend) and run the headless tests in Node.
#
#   ./probe_wasm.sh [threads|nothreads] [build dir]
#
# Needs Emscripten (emcmake on the PATH; tested with the 3.1.6 of Ubuntu 24.04: apt install emscripten) and Node.
# Node 22 and this old Emscripten need synchronous instantiation (-sWASM_ASYNC_COMPILATION=0): a current emsdk does not.
# A cross build cannot run ravenc, so only the targets listed below are built (not test_xmlc / test_xmlcpp).
set -e
MODE=${1:-threads}
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT="$HERE/../.."
BUILD=${2:-/tmp/raven-wasm-$MODE}
LINK="-sWASM_ASYNC_COMPILATION=0 -sEXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1 -sTOTAL_STACK=5MB"
CFLAGS=""
if [ "$MODE" = threads ]; then
  CFLAGS="-pthread"
  LINK="$LINK -pthread -sPTHREAD_POOL_SIZE=8 -sINITIAL_MEMORY=128MB"
fi
TESTS="test_layout test_widgets test_xml test_memory test_animation test_text_editor test_shell test_dialogs test_themes test_markdown test_tools test_locale test_run test_touch test_touch_focus test_single_window test_threads test_editor_features"

emcmake cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DRAVEN_PLATFORM=null -DRAVEN_BACKEND_VULKAN=OFF -DRAVEN_BUILD_WEB=OFF \
  -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_BUILD_TESTS=ON \
  -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_EXE_LINKER_FLAGS="$LINK" >/dev/null
for t in $TESTS; do ninja -C "$BUILD" $t >/dev/null; done

# tests/test_dock_tearing.c has main(int, char**), which Emscripten 3.1.6 does not link (it finds no main): use a main(void) copy
sed 's|int main(int argc, char\*\* argv) {|int main(void) { int argc = 1; char** argv = 0; (void)argv;|' "$ROOT/tests/test_dock_tearing.c" > "$BUILD/test_dock_tearing_main.c"
emcc -O2 -std=gnu11 -I"$ROOT/include" $CFLAGS $LINK "$BUILD/test_dock_tearing_main.c" "$BUILD/libraven.a" -o "$BUILD/bin/test_dock_tearing.js"

FAIL=0
for t in $TESTS test_dock_tearing; do
  out=$(timeout 300 node "$BUILD/bin/$t.js" 2>&1) && code=0 || code=$?
  printf '%-22s exit=%s  %s\n' "$t" "$code" "$(echo "$out" | tail -1 | cut -c1-80)"
  [ "$code" = 0 ] || FAIL=1
done
exit $FAIL
