// Raven SDK Installer - the C/C++ compilers of the machine.
//
// Nothing is assumed from a name alone. A compiler is reported with what it
// says about itself (version, target triple) and, when `verify` is set, after
// it has compiled and linked a small C program:
//
//   MSVC         vswhere.exe finds the Visual Studio installations that have
//                the x64 C++ tools; for each one the toolset version is read,
//                cl.exe, the Windows SDK (windows.h, kernel32.lib) are checked,
//                then vcvars64.bat + cl compile and link a program. A cl.exe in
//                the PATH of a Developer Prompt is reported too.
//   MinGW-w64    gcc in the PATH and in the usual folders (MSYS2, w64devkit,
//                Chocolatey, Scoop, WinLibs); -dumpmachine must be
//                x86_64-*-mingw32.
//   GCC, Clang   (Linux) gcc, cc, clang and their versioned names in the PATH.
//   Apple Clang  only when `xcode-select -p` names a developer folder, so that
//                the /usr/bin/clang shim never pops up the installation dialog.
//
// Families map to the targets of the payload: msvc -> msvc-*, mingw ->
// mingw-*, gcc/clang -> gnu-*, appleclang -> apple-*.
#pragma once

#include "../core/util.hpp"

#include <string>
#include <vector>

namespace rinst {

struct Compiler {
	std::string family;   // "msvc", "mingw", "gcc", "clang", "appleclang"
	std::string name;     // "MSVC 19.40 (Visual Studio Community 2022 17.10)"
	std::string version;  // "19.40.33811", "13.2.0"
	std::string machine;  // target triple or architecture ("x86_64-w64-mingw32", "x64")
	std::string path;     // the compiler program
	std::string location; // install folder, when meaningful
	bool usable = false;  // targets the host (x64 / arm64) and found complete
	bool verified = false;// a test program was built
	std::string note;     // why it is not usable, or what was checked
};

struct CompilerDetection {
	std::vector<Compiler> compilers;
	std::vector<std::string> log;
	/** Target families a usable compiler was found for ("msvc", "mingw", "gnu", "apple"). */
	std::vector<std::string> families() const;
	bool has_family(const std::string& family) const;
};

/** The target family of a compiler family ("gcc" -> "gnu"). */
std::string target_family_of(const std::string& compiler_family);

/** Slow (seconds, with verify): call it from a worker thread. */
CompilerDetection detect_compilers(bool verify);

} // namespace rinst
