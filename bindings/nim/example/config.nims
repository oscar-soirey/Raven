# The example finds the package in the folder above, and the shared library
# of Raven in RAVEN_LIB_DIR (linked, and found again when it runs).
import std/os

switch("path", thisDir() / ".." / "src")
let libDir = getEnv("RAVEN_LIB_DIR")
if libDir.len > 0:
  switch("passL", "-L" & libDir)
  when not defined(windows):
    switch("passL", "-Wl,-rpath," & libDir)
