@echo off
rem ---------------------------------------------------------------------------
rem  Raven - examples\build.bat
rem  Builds the example projects that are not part of the CMake build of Raven:
rem
rem    languages\hello_c        main.c           CMake
rem    languages\hello_cpp      main.cpp         CMake
rem    languages\hello_rust     src\main.rs      Cargo
rem    languages\hello_python   main.py          checked (Python runs it)
rem    languages\hello_go       main.go          go build
rem    languages\hello_csharp   Program.cs       dotnet
rem    languages\hello_zig      src\main.zig     zig build
rem    languages\hello_lua      main.lua         checked (LuaJIT runs it)
rem    bevy_editor              ui\src\editor.rs Cargo (Bevy 0.16)
rem
rem  Each one loads its interface, main.xml (editor.xml for the Bevy editor),
rem  when it starts.
rem
rem  (The C examples - studio, editor, glass... - are built by ..\build.bat.)
rem
rem    build.bat [configuration] [options]
rem
rem    configuration     Debug or Release (default)
rem    --raven DIR       a Raven package (bin\raven.dll, lib\, include\).
rem                      Default: RAVEN_PACKAGE if set, else Raven is built with
rem                      ..\build.bat and installed into ..\build\package
rem    --only NAMES      build these only, separated by commas:
rem                      c,cpp,rust,python,go,csharp,zig,lua,bevy
rem    --no-bevy         the Bevy editor without Bevy (its software renderer only):
rem                      much faster, nothing to download
rem    --clean           remove what the examples built before
rem    --run NAME        start one after the build: c, cpp, rust, python, go,
rem                      csharp, zig, lua, bevy (Bevy), bevy-software
rem    --no-pause        do not wait for a key when something failed
rem
rem  Examples:
rem    build.bat
rem    build.bat --only rust,bevy --run bevy
rem    build.bat --raven C:\raven-0.1-windows-x64-msvc --no-bevy
rem
rem  A language whose tools are not installed (cargo, go, dotnet, zig) is
rem  skipped, not failed. The first build of the Bevy editor downloads and
rem  compiles Bevy: several minutes.
rem ---------------------------------------------------------------------------
setlocal EnableExtensions EnableDelayedExpansion

set "HERE=%~dp0"
if "%HERE:~-1%"=="\" set "HERE=%HERE:~0,-1%"
for %%I in ("%HERE%\..") do set "ROOT=%%~fI"

set "CONFIG=Release"
set "PKG="
if defined RAVEN_PACKAGE set "PKG=%RAVEN_PACKAGE%"
set "ONLY=c,cpp,rust,python,go,csharp,zig,lua,bevy"
set "NO_BEVY=0"
set "CLEAN=0"
set "RUN="
set "NO_PAUSE=0"
if defined RAVEN_NO_PAUSE set "NO_PAUSE=1"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="debug"      ( set "CONFIG=Debug" & shift & goto parse )
if /I "%~1"=="release"    ( set "CONFIG=Release" & shift & goto parse )
if /I "%~1"=="--raven"    ( set "PKG=%~f2" & shift & shift & goto parse )
if /I "%~1"=="--only"     ( set "ONLY=%~2" & shift & shift & goto parse )
if /I "%~1"=="--no-bevy"  ( set "NO_BEVY=1" & shift & goto parse )
if /I "%~1"=="--clean"    ( set "CLEAN=1" & shift & goto parse )
if /I "%~1"=="--run"      ( set "RUN=%~2" & shift & shift & goto parse )
if /I "%~1"=="--no-pause" ( set "NO_PAUSE=1" & shift & goto parse )
if /I "%~1"=="--help"     goto usage
if /I "%~1"=="-h"         goto usage
echo [examples] unknown option: %~1
echo.
goto usage
:parsed

set "CARGO_PROFILE=--release"
set "CARGO_DIR=release"
if /I "%CONFIG%"=="Debug" ( set "CARGO_PROFILE=" & set "CARGO_DIR=debug" )

rem ---- the Raven package ----------------------------------------------------
if defined PKG goto have_package
where cmake >nul 2>nul
if errorlevel 1 (
	echo [examples] cmake was not found in the PATH: install CMake 3.16 or newer,
	echo            or give a Raven package with --raven DIR.
	goto failed
)
echo [examples] building Raven ^(..\build.bat %CONFIG%^)
call "%ROOT%\build.bat" %CONFIG% --no-pause
if errorlevel 1 goto failed
set "PKG=%ROOT%\build\package"
echo [examples] installing Raven into %PKG%
cmake --install "%ROOT%\build" --config %CONFIG% --prefix "%PKG%"
if errorlevel 1 goto failed
:have_package
if "%PKG:~-1%"=="\" set "PKG=%PKG:~0,-1%"
if not exist "%PKG%\lib\cmake\raven\ravenConfig.cmake" (
	echo [examples] "%PKG%" is not a Raven package: lib\cmake\raven\ravenConfig.cmake is missing.
	goto failed
)

rem  What every tool needs: the library, the headers, the CMake package
set "RAVEN_LIB_DIR=%PKG%\lib"
set "PATH=%PKG%\bin;%PATH%"
set "CMAKE_PREFIX_PATH=%PKG%;%CMAKE_PREFIX_PATH%"
set "CGO_ENABLED=1"
set "CGO_CFLAGS=-I"%PKG%\include""
set "CGO_LDFLAGS=-L"%PKG%\lib" -L"%PKG%\bin""
set "BINDINGS=%ROOT%\bindings"
if exist "%PKG%\share\raven\bindings" set "BINDINGS=%PKG%\share\raven\bindings"
set "PYTHONPATH=%BINDINGS%\python;%PYTHONPATH%"
set "LUA_PATH=%BINDINGS%\luajit\?.lua;%LUA_PATH%;;"
echo [examples] Raven package: %PKG%
echo.

set "LANG_DIR=%HERE%\languages"
set "FAILED=0"
for %%N in (c cpp rust python go csharp zig lua bevy) do set "R_%%N=not asked"
for %%N in (%ONLY:,= %) do (
	if not defined R_%%N (
		echo [examples] unknown example in --only: %%N
		goto failed
	)
	call :build_%%N
)

rem ---- what happened --------------------------------------------------------
echo.
echo [examples] ------------------------------------------------------------
for %%N in (c cpp rust python go csharp zig lua bevy) do echo   %%N	!R_%%N!
echo [examples] ------------------------------------------------------------

if defined RUN goto run
if "%FAILED%"=="1" goto failed
exit /b 0

rem ===========================================================================
rem  One subroutine per example. Each sets R_<name> to OK, FAILED or skipped.
rem ===========================================================================

:build_c
call :cmake_project hello_c c
goto :eof

:build_cpp
call :cmake_project hello_cpp cpp
goto :eof

rem  :cmake_project <folder> <name>
:cmake_project
set "P=%LANG_DIR%\%~1"
echo [examples] %~1 ^(CMake^)
where cmake >nul 2>nul
if errorlevel 1 ( set "R_%~2=skipped: cmake not found" & goto :eof )
if "%CLEAN%"=="1" if exist "%P%\build" rmdir /s /q "%P%\build"
cmake -S "%P%" -B "%P%\build" -DCMAKE_PREFIX_PATH="%PKG%" -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 ( set "R_%~2=FAILED (configure)" & set "FAILED=1" & goto :eof )
cmake --build "%P%\build" --config %CONFIG% --parallel
if errorlevel 1 ( set "R_%~2=FAILED" & set "FAILED=1" & goto :eof )
rem  the DLL next to the program, so that it also starts from the Explorer
if exist "%P%\build\%CONFIG%\%~1.exe" call :copy_dll "%P%\build\%CONFIG%"
if exist "%P%\build\%~1.exe" call :copy_dll "%P%\build"
set "R_%~2=OK"
goto :eof

:build_rust
set "P=%LANG_DIR%\hello_rust"
echo [examples] hello_rust ^(Cargo^)
where cargo >nul 2>nul
if errorlevel 1 ( set "R_rust=skipped: cargo not found (https://rustup.rs)" & goto :eof )
pushd "%P%"
if "%CLEAN%"=="1" cargo clean
cargo build %CARGO_PROFILE%
set "E=%ERRORLEVEL%"
popd
if not "%E%"=="0" ( set "R_rust=FAILED" & set "FAILED=1" & goto :eof )
call :copy_dll "%P%\target\%CARGO_DIR%"
set "R_rust=OK"
goto :eof

:build_python
set "P=%LANG_DIR%\hello_python"
echo [examples] hello_python ^(checked^)
where python >nul 2>nul
if errorlevel 1 ( set "R_python=skipped: python not found" & goto :eof )
python -m py_compile "%P%\main.py"
if errorlevel 1 ( set "R_python=FAILED" & set "FAILED=1" & goto :eof )
set "R_python=OK (run it: --run python)"
goto :eof

:build_lua
set "P=%LANG_DIR%\hello_lua"
echo [examples] hello_lua ^(checked^)
where luajit >nul 2>nul
if errorlevel 1 ( set "R_lua=skipped: luajit not found" & goto :eof )
luajit -e "assert(loadfile([[%P%\main.lua]]))"
if errorlevel 1 ( set "R_lua=FAILED" & set "FAILED=1" & goto :eof )
set "R_lua=OK (run it: --run lua)"
goto :eof

:build_go
set "P=%LANG_DIR%\hello_go"
echo [examples] hello_go ^(go build^)
where go >nul 2>nul
if errorlevel 1 ( set "R_go=skipped: go not found (https://go.dev/dl)" & goto :eof )
where gcc >nul 2>nul
if errorlevel 1 ( set "R_go=skipped: cgo needs gcc in the PATH (MinGW-w64)" & goto :eof )
pushd "%P%"
if "%CLEAN%"=="1" if exist hello_go.exe del /q hello_go.exe
go build -o hello_go.exe .
set "E=%ERRORLEVEL%"
popd
if not "%E%"=="0" ( set "R_go=FAILED" & set "FAILED=1" & goto :eof )
call :copy_dll "%P%"
set "R_go=OK"
goto :eof

:build_csharp
set "P=%LANG_DIR%\hello_csharp"
echo [examples] hello_csharp ^(dotnet^)
where dotnet >nul 2>nul
if errorlevel 1 ( set "R_csharp=skipped: dotnet not found (.NET 8 SDK)" & goto :eof )
pushd "%P%"
if "%CLEAN%"=="1" dotnet clean -c %CONFIG% >nul
dotnet build -c %CONFIG%
set "E=%ERRORLEVEL%"
popd
if not "%E%"=="0" ( set "R_csharp=FAILED" & set "FAILED=1" & goto :eof )
for /d %%D in ("%P%\bin\%CONFIG%\*") do call :copy_dll "%%~fD"
set "R_csharp=OK"
goto :eof

:build_zig
set "P=%LANG_DIR%\hello_zig"
echo [examples] hello_zig ^(zig build^)
where zig >nul 2>nul
if errorlevel 1 ( set "R_zig=skipped: zig not found (https://ziglang.org, 0.14)" & goto :eof )
pushd "%P%"
if "%CLEAN%"=="1" if exist zig-out rmdir /s /q zig-out
if "%CLEAN%"=="1" if exist .zig-cache rmdir /s /q .zig-cache
set "ZIG_MODE="
if /I "%CONFIG%"=="Release" set "ZIG_MODE=-Doptimize=ReleaseFast"
zig build %ZIG_MODE%
set "E=%ERRORLEVEL%"
popd
if not "%E%"=="0" ( set "R_zig=FAILED" & set "FAILED=1" & goto :eof )
call :copy_dll "%P%\zig-out\bin"
set "R_zig=OK"
goto :eof

:build_bevy
set "P=%HERE%\bevy_editor"
echo [examples] bevy_editor ^(Cargo^)
where cargo >nul 2>nul
if errorlevel 1 ( set "R_bevy=skipped: cargo not found (https://rustup.rs)" & goto :eof )
pushd "%P%"
if "%CLEAN%"=="1" cargo clean
if "%NO_BEVY%"=="1" (
	echo [examples] without Bevy: the editor and its software renderer only
	cargo build %CARGO_PROFILE% -p editor_ui
) else (
	echo [examples] the first build downloads and compiles Bevy: several minutes
	cargo build %CARGO_PROFILE%
)
set "E=%ERRORLEVEL%"
popd
if not "%E%"=="0" ( set "R_bevy=FAILED" & set "FAILED=1" & goto :eof )
call :copy_dll "%P%\target\%CARGO_DIR%"
if "%NO_BEVY%"=="1" ( set "R_bevy=OK (software renderer only)" ) else ( set "R_bevy=OK" )
goto :eof

rem  :copy_dll <folder>  puts raven.dll next to a program
:copy_dll
if exist "%PKG%\bin\raven.dll" if exist "%~1" copy /y "%PKG%\bin\raven.dll" "%~1\" >nul
goto :eof

rem ===========================================================================
rem  --run NAME: from the folder of the example (strings.csv is read from there)
rem ===========================================================================
:run
set "E=1"
if /I "%RUN%"=="c"             call :run_cmake hello_c
if /I "%RUN%"=="cpp"           call :run_cmake hello_cpp
if /I "%RUN%"=="rust"          call :run_in "%LANG_DIR%\hello_rust" "target\%CARGO_DIR%\hello_rust.exe"
if /I "%RUN%"=="go"            call :run_in "%LANG_DIR%\hello_go" "hello_go.exe"
if /I "%RUN%"=="zig"           call :run_in "%LANG_DIR%\hello_zig" "zig-out\bin\hello_zig.exe"
if /I "%RUN%"=="bevy"          call :run_in "%HERE%\bevy_editor" "target\%CARGO_DIR%\raven-bevy-editor.exe"
if /I "%RUN%"=="bevy-software" call :run_software
if /I "%RUN%"=="csharp" (
	pushd "%LANG_DIR%\hello_csharp" & dotnet run -c %CONFIG% --no-build & set "E=!ERRORLEVEL!" & popd
)
if /I "%RUN%"=="python" (
	pushd "%LANG_DIR%\hello_python" & python main.py & set "E=!ERRORLEVEL!" & popd
)
if /I "%RUN%"=="lua" (
	pushd "%LANG_DIR%\hello_lua" & luajit main.lua & set "E=!ERRORLEVEL!" & popd
)
if "%FAILED%"=="1" goto failed
exit /b %E%

:run_cmake
set "EXE=%LANG_DIR%\%~1\build\%CONFIG%\%~1.exe"
if not exist "%EXE%" set "EXE=%LANG_DIR%\%~1\build\%~1.exe"
call :run_in "%LANG_DIR%\%~1" "%EXE%"
goto :eof

:run_software
set "EXE=target\%CARGO_DIR%\raven-scene-editor-software.exe"
if not exist "%HERE%\bevy_editor\%EXE%" set "EXE=target\%CARGO_DIR%\raven-bevy-editor.exe"
call :run_in "%HERE%\bevy_editor" "%EXE%" --software
goto :eof

rem  :run_in <folder> <program> [arguments]
:run_in
pushd "%~1"
if not exist "%~2" (
	echo [examples] %~2 was not built: build it first ^(without --only, or with --only and its name^).
	popd
	set "E=1"
	goto :eof
)
echo [examples] starting %~2
"%~2" %3
set "E=%ERRORLEVEL%"
popd
goto :eof

:failed
echo.
echo [examples] something FAILED: see above.
if "%NO_PAUSE%"=="0" pause
exit /b 1

:usage
echo Usage: build.bat [Debug^|Release] [options]
echo.
echo   --raven DIR       a Raven package; default: RAVEN_PACKAGE, else ..\build.bat + install
echo   --only NAMES      c,cpp,rust,python,go,csharp,zig,lua,bevy ^(comma separated^)
echo   --no-bevy         the Bevy editor with its software renderer only ^(no Bevy to compile^)
echo   --clean           remove what the examples built before
echo   --run NAME        c, cpp, rust, python, go, csharp, zig, lua, bevy, bevy-software
echo   --no-pause        do not wait for a key when something failed
exit /b 1
