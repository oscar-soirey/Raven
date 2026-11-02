@echo off
rem ---------------------------------------------------------------------------
rem  Raven - bindings\package.bat
rem  Makes the package files of the bindings, ready to be published. It
rem  publishes nothing.
rem
rem    bindings\package.bat [options]
rem
rem    --prefix DIR     a shared build of Raven for Windows (bin\raven.dll)
rem                     instead of building one
rem    --linux DIR      also the Linux packages, from a shared Raven package for
rem                     Linux (raven-linux-x64-shared of a release, unzipped)
rem    --macos DIR      also the macOS packages, from a shared Raven package for
rem                     macOS (raven-macos-universal-shared of a release, unzipped)
rem    --no-pause       do not wait for a key when something fails
rem
rem  Result, in dist\bindings\:
rem    pypi\       raven_gui-<version>-py3-none-win_amd64.whl (and the Linux and
rem                macOS wheels with --linux / --macos)     needs Python
rem    nuget\      Raven.Gui.<version>.nupkg                  needs the .NET SDK
rem    crates\     raven-gui-<version>.crate                 needs cargo (Rust)
rem    maven\      raven-gui-<version>.jar (and -sources, -javadoc; the
rem                libraries inside)                          needs Maven and a JDK 22+
rem    luarocks\   raven-gui-<version>-1.rockspec
rem  A part whose tool is not installed is skipped, with a message.
rem
rem  Without --prefix, Raven is built first (shared, Release, Visual C++, the
rem  C runtime linked statically) in build\bindings\: CMake and Visual Studio
rem  are needed, as for build.bat.
rem
rem  To publish them afterwards: bindings\PUBLISHING.md.
rem ---------------------------------------------------------------------------
setlocal EnableExtensions

set "BINDINGS=%~dp0"
if "%BINDINGS:~-1%"=="\" set "BINDINGS=%BINDINGS:~0,-1%"
for %%i in ("%BINDINGS%\..") do set "ROOT=%%~fi"

set "PREFIX="
set "LINUX="
set "MACOS="
set "NO_PAUSE=0"
if defined RAVEN_NO_PAUSE set "NO_PAUSE=1"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="--prefix"   ( set "PREFIX=%~f2" & shift & shift & goto parse )
if /I "%~1"=="--linux"    ( set "LINUX=%~f2" & shift & shift & goto parse )
if /I "%~1"=="--macos"    ( set "MACOS=%~f2" & shift & shift & goto parse )
if /I "%~1"=="--no-pause" ( set "NO_PAUSE=1" & shift & goto parse )
if /I "%~1"=="--help"     goto usage
if /I "%~1"=="-h"         goto usage
echo [package] unknown option: %~1
echo.
goto usage

:parsed
set "OUT=%ROOT%\dist\bindings"
set "MADE="
set "SKIPPED="

rem  the version, from CMakeLists.txt
set "VERSION=0.0.0"
for /f "tokens=3" %%v in ('findstr /r /c:"^project.raven VERSION" "%ROOT%\CMakeLists.txt"') do set "VERSION=%%v"
echo [package] Raven %VERSION%

if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%"

rem ---- the shared library for Windows -----------------------------------------
if defined PREFIX goto have_prefix
where cmake >nul 2>nul
if errorlevel 1 (
	echo [package] cmake was not found in the PATH.
	echo           Install CMake, start this script from a "Developer Command Prompt for
	echo           Visual Studio", or give a built Raven with --prefix.
	goto failed
)
set "BUILD=%ROOT%\build\bindings"
set "PREFIX=%BUILD%\install"
echo.
echo [package] building Raven ^(shared, Release^) in "%BUILD%"
cmake -S "%ROOT%" -B "%BUILD%" -A x64 -DRAVEN_BUILD_SHARED=ON -DRAVEN_BUILD_TESTS=OFF -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TOOLS=OFF -DRAVEN_MSVC_STATIC_RUNTIME=ON
if errorlevel 1 goto failed
cmake --build "%BUILD%" --config Release --parallel
if errorlevel 1 goto failed
if exist "%PREFIX%" rmdir /s /q "%PREFIX%"
cmake --install "%BUILD%" --config Release --prefix "%PREFIX%"
if errorlevel 1 goto failed

:have_prefix
if not exist "%PREFIX%\bin\raven.dll" (
	echo [package] "%PREFIX%\bin\raven.dll" was not found: --prefix must be a shared build of Raven.
	goto failed
)

rem ---- PyPI --------------------------------------------------------------------
echo.
echo [package] PyPI
set "PYTHON="
where py >nul 2>nul
if not errorlevel 1 set "PYTHON=py -3"
if not defined PYTHON (
	where python >nul 2>nul
	if not errorlevel 1 set "PYTHON=python"
)
if not defined PYTHON (
	echo [package] Python was not found: the wheels are skipped.
	set "SKIPPED=%SKIPPED% pypi"
	goto pypi_done
)
mkdir "%OUT%\pypi"
%PYTHON% "%BINDINGS%\python\build_wheel.py" --prefix "%PREFIX%" --out "%OUT%\pypi"
if errorlevel 1 goto failed
if defined LINUX (
	%PYTHON% "%BINDINGS%\python\build_wheel.py" --prefix "%LINUX%" --out "%OUT%\pypi"
	if errorlevel 1 goto failed
)
if defined MACOS (
	%PYTHON% "%BINDINGS%\python\build_wheel.py" --prefix "%MACOS%" --out "%OUT%\pypi"
	if errorlevel 1 goto failed
)
set "MADE=%MADE% pypi"
:pypi_done

rem ---- NuGet -------------------------------------------------------------------
echo.
echo [package] NuGet
where dotnet >nul 2>nul
if errorlevel 1 (
	echo [package] the .NET SDK was not found ^(https://dot.net^): the NuGet package is skipped.
	set "SKIPPED=%SKIPPED% nuget"
	goto nuget_done
)
set "NATIVE=%BINDINGS%\csharp\native"
for %%r in (win-x64 linux-x64 osx) do if exist "%NATIVE%\%%r" rmdir /s /q "%NATIVE%\%%r"
mkdir "%NATIVE%\win-x64"
copy /y "%PREFIX%\bin\raven.dll" "%NATIVE%\win-x64\" >nul
if defined LINUX call :native_unix "%LINUX%" linux-x64 libraven.so
if errorlevel 1 goto failed
if defined MACOS call :native_unix "%MACOS%" osx libraven.dylib
if errorlevel 1 goto failed
dotnet pack "%BINDINGS%\csharp\Raven.Gui.csproj" -c Release -o "%OUT%\nuget" --nologo
if errorlevel 1 goto failed
set "MADE=%MADE% nuget"
:nuget_done

rem ---- crates.io ---------------------------------------------------------------
echo.
echo [package] crates.io
where cargo >nul 2>nul
if errorlevel 1 (
	echo [package] cargo was not found ^(https://rustup.rs^): the crates are skipped.
	set "SKIPPED=%SKIPPED% crates"
	goto crates_done
)
set "CARGO_TARGET=%ROOT%\build\bindings-cargo"
if exist "%CARGO_TARGET%\package" rmdir /s /q "%CARGO_TARGET%\package"
pushd "%BINDINGS%\rust"
cargo package --allow-dirty --target-dir "%CARGO_TARGET%"
if errorlevel 1 ( popd & goto failed )
popd
mkdir "%OUT%\crates"
copy /y "%CARGO_TARGET%\package\*.crate" "%OUT%\crates\" >nul
set "MADE=%MADE% crates"
:crates_done

rem ---- Maven -------------------------------------------------------------------
echo.
echo [package] Maven
where mvn >nul 2>nul
if errorlevel 1 (
	echo [package] Maven was not found ^(https://maven.apache.org, and a JDK 22 or later^): the jar is skipped.
	set "SKIPPED=%SKIPPED% maven"
	goto maven_done
)
call :java_natives
if errorlevel 1 goto failed
call mvn -B -q -f "%BINDINGS%\java\pom.xml" clean package
if errorlevel 1 goto failed
mkdir "%OUT%\maven"
copy /y "%BINDINGS%\java\target\raven-gui-*.jar" "%OUT%\maven\" >nul
set "MADE=%MADE% maven"
:maven_done

rem ---- LuaRocks ----------------------------------------------------------------
echo.
echo [package] LuaRocks
mkdir "%OUT%\luarocks"
copy /y "%BINDINGS%\luajit\raven-gui-*.rockspec" "%OUT%\luarocks\" >nul
if errorlevel 1 goto failed
set "MADE=%MADE% luarocks"

rem ---- done --------------------------------------------------------------------
echo.
echo ===========================================================================
echo [package] done:%MADE%
if defined SKIPPED echo           skipped ^(tool not installed^):%SKIPPED%
echo           in "%OUT%"
dir /s /b "%OUT%"
endlocal
exit /b 0

rem ---------------------------------------------------------------------------
rem  :native_unix <Raven package> <rid> <library name>
rem  Copies the library of a Linux or macOS package to %NATIVE%\<rid>\
rem  (csharp\native\ or the resources of the jar)
rem  (a package unzipped on Windows may have lost the links libraven.so -> libraven.so.0.1.0)
rem ---------------------------------------------------------------------------
:native_unix
set "SRC=%~1"
set "RID=%~2"
set "LIB=%~3"
mkdir "%NATIVE%\%RID%"
set "FOUND="
for %%d in (lib lib64 bin) do (
	if not defined FOUND if exist "%SRC%\%%d\%LIB%" (
		copy /y "%SRC%\%%d\%LIB%" "%NATIVE%\%RID%\%LIB%" >nul
		set "FOUND=1"
	)
)
if not defined FOUND for %%d in (lib lib64) do (
	for %%f in ("%SRC%\%%d\libraven.so.*.*.*" "%SRC%\%%d\libraven.*.*.*.dylib") do (
		if not defined FOUND (
			copy /y "%%~f" "%NATIVE%\%RID%\%LIB%" >nul
			set "FOUND=1"
		)
	)
)
if not defined FOUND (
	echo [package] no %LIB% in "%SRC%" ^(it must be a shared build of Raven^)
	exit /b 1
)
exit /b 0

rem ---------------------------------------------------------------------------
rem  :java_natives
rem  The libraries of the jar: java\src\main\resources\raven\native\<platform>\
rem  (R.java copies the one of its platform out of the jar when it starts)
rem ---------------------------------------------------------------------------
:java_natives
set "NATIVE=%BINDINGS%\java\src\main\resources\raven\native"
for %%r in (windows-x64 linux-x64 macos) do if exist "%NATIVE%\%%r" rmdir /s /q "%NATIVE%\%%r"
mkdir "%NATIVE%\windows-x64"
copy /y "%PREFIX%\bin\raven.dll" "%NATIVE%\windows-x64\" >nul
if errorlevel 1 exit /b 1
if defined LINUX call :native_unix "%LINUX%" linux-x64 libraven.so
if errorlevel 1 exit /b 1
if defined MACOS call :native_unix "%MACOS%" macos libraven.dylib
if errorlevel 1 exit /b 1
exit /b 0

:failed
echo.
echo [package] FAILED.
if "%NO_PAUSE%"=="0" pause
endlocal
exit /b 1

:usage
echo Usage: bindings\package.bat [--prefix DIR] [--linux DIR] [--macos DIR] [--no-pause]
echo.
echo   Makes the packages of the bindings in dist\bindings\ ^(pypi, nuget, crates,
echo   maven, luarocks^). Publishes nothing.
echo.
echo   --prefix DIR   a shared build of Raven for Windows, instead of building one
echo   --linux DIR    also the Linux packages, from a shared Raven package for Linux
echo   --macos DIR    also the macOS packages, from a shared Raven package for macOS
echo   --no-pause     do not wait for a key when something fails
exit /b 1
