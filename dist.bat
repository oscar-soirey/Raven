@echo off
rem ---------------------------------------------------------------------------
rem  Raven - dist.bat
rem  Builds what is given to people who use Raven without building it, the way
rem  build.bat builds (generate-cmake.bat, then build.bat): one folder with
rem  the library of Visual C++ and of MinGW-w64, static and
rem  shared, each a complete package (headers, library, designer, docs).
rem
rem    dist.bat [msvc | mingw | all] [options]
rem
rem    msvc                Visual C++ only (the Visual Studio build.bat uses)
rem    mingw               MinGW-w64 only (gcc, and ninja or mingw32-make, in the PATH)
rem    all                 both compilers; one that is not installed is skipped (default)
rem    --test              run the tests of each build (slow: eight builds)
rem    --no-pause          do not wait for a key when something fails
rem
rem  Result:
rem    dist\raven-<version>\
rem        msvc-static\     include\, lib\raven.lib, lib\cmake\raven, bin\ (designer), share\raven\
rem        msvc-shared\     include\, bin\raven.dll, lib\raven.lib (import library), lib\cmake\raven, ...
rem        mingw-static\    include\, lib\libraven.a, lib\cmake\raven, ...
rem        mingw-shared\    include\, bin\raven.dll, lib\libraven.dll.a (import library), lib\cmake\raven, ...
rem    dist\raven-<version>.zip
rem
rem  And, ready to be attached to a release on GitHub, one archive per library:
rem    dist-release\windows-x64-msvc-static.zip
rem    dist-release\windows-x64-msvc-shared.zip
rem    dist-release\windows-x64-mingw-static.zip
rem    dist-release\windows-x64-mingw-shared.zip
rem    dist-release\SHA256SUMS.txt                  the checksums of the archives
rem  Each archive holds one folder of the same name (windows-x64-msvc-static\...).
rem
rem  Each of the four folders is an install prefix: find_package(raven) with
rem  CMAKE_PREFIX_PATH set to it. The libraries of one compiler are not for the
rem  other: a program built with Visual C++ takes msvc-*, one built with MinGW
rem  mingw-*. A DLL itself can be loaded by anything (Python, C#, Rust...).
rem  The build trees are in build\dist\.
rem ---------------------------------------------------------------------------
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "WHAT=all"
set "TEST=0"
set "NO_PAUSE=0"
if defined RAVEN_NO_PAUSE set "NO_PAUSE=1"

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="msvc"       ( set "WHAT=msvc" & shift & goto parse )
if /I "%~1"=="mingw"      ( set "WHAT=mingw" & shift & goto parse )
if /I "%~1"=="all"        ( set "WHAT=all" & shift & goto parse )
if /I "%~1"=="--test"     ( set "TEST=1" & shift & goto parse )
if /I "%~1"=="--no-test"  ( set "TEST=0" & shift & goto parse )
if /I "%~1"=="--no-pause" ( set "NO_PAUSE=1" & shift & goto parse )
if /I "%~1"=="--help"     goto usage
if /I "%~1"=="-h"         goto usage
echo [dist] unknown option: %~1
echo.
goto usage

:parsed
where cmake >nul 2>nul
if errorlevel 1 (
	echo [dist] cmake was not found in the PATH.
	echo        Install CMake 3.16 or newer from https://cmake.org/download, or start
	echo        this script from a "Developer Command Prompt for Visual Studio".
	goto failed
)

set "VERSION=0.0.0"
for /f "tokens=3" %%v in ('findstr /r /c:"^project.raven VERSION" "%ROOT%\CMakeLists.txt"') do set "VERSION=%%v"

rem  MinGW-w64: gcc, and a make program (Ninja when it is there, else mingw32-make)
set "MINGW=1"
set "MINGW_GENERATOR="
where gcc >nul 2>nul
if errorlevel 1 set "MINGW=0"
where ninja >nul 2>nul
if not errorlevel 1 set "MINGW_GENERATOR=Ninja"
if not defined MINGW_GENERATOR (
	where mingw32-make >nul 2>nul
	if not errorlevel 1 set "MINGW_GENERATOR=MinGW Makefiles"
)
if not defined MINGW_GENERATOR set "MINGW=0"
if "%MINGW%"=="0" if /I "%WHAT%"=="mingw" (
	echo [dist] MinGW-w64 was not found in the PATH: gcc, and ninja or mingw32-make.
	goto failed
)

set "MADE="
set "SKIPPED="
call :edition
if errorlevel 1 goto failed

:done
echo.
echo ===========================================================================
if not defined MADE (
	echo [dist] nothing was built: no compiler could be used.
	goto failed
)
call :checksums "%ROOT%\dist-release"
echo [dist] done:%MADE%
if defined SKIPPED echo        skipped:%SKIPPED%
echo        packages in "%ROOT%\dist"
echo        archives for a release in "%ROOT%\dist-release"
endlocal
exit /b 0

rem ---------------------------------------------------------------------------
rem  :edition   the four packages, then the zip
rem ---------------------------------------------------------------------------
:edition
set "NAME=raven-%VERSION%"
set "OUT=%ROOT%\dist\%NAME%"
set "BUILT=0"
if exist "%OUT%" rmdir /s /q "%OUT%"
if exist "%OUT%.zip" del /q "%OUT%.zip"

if /I "%WHAT%"=="mingw" goto edition_mingw
call :variant msvc static
if errorlevel 2 goto edition_mingw
if errorlevel 1 exit /b 1
call :variant msvc shared
if errorlevel 1 exit /b 1
:edition_mingw
if /I "%WHAT%"=="msvc" goto edition_zip
if "%MINGW%"=="0" (
	echo.
	echo [dist] MinGW-w64 was not found in the PATH ^(gcc, and ninja or mingw32-make^): skipped.
	set "SKIPPED=%SKIPPED% mingw"
	goto edition_zip
)
call :variant mingw static
if errorlevel 1 exit /b 1
call :variant mingw shared
if errorlevel 1 exit /b 1

:edition_zip
if "%BUILT%"=="0" exit /b 0
pushd "%ROOT%\dist"
cmake -E tar cf "%NAME%.zip" --format=zip "%NAME%"
if errorlevel 1 ( popd & exit /b 1 )
popd
echo [dist] dist\%NAME%.zip
set "MADE=%MADE% %NAME%"
exit /b 0

rem ---------------------------------------------------------------------------
rem  :variant <msvc|mingw> <static|shared>
rem  Generates (generate-cmake.bat), builds (build.bat) and installs one
rem  package. Returns 0 when it was made, 1 on a failure, 2 when Visual C++
rem  cannot be used and "all" was asked (its packages are then skipped).
rem ---------------------------------------------------------------------------
:variant
set "TOOL=%~1"
set "KIND=%~2"
set "BUILD=%ROOT%\build\dist\%TOOL%-%KIND%"
set "PREFIX=%OUT%\%TOOL%-%KIND%"
echo.
echo ===========================================================================
echo  %NAME%\%TOOL%-%KIND%
echo ===========================================================================

set "GENERATE_ARGS=--no-pause --fresh --no-examples --build-dir "%BUILD%""
if "%TEST%"=="0" set "GENERATE_ARGS=%GENERATE_ARGS% --no-tests"
if /I "%KIND%"=="shared" set "GENERATE_ARGS=%GENERATE_ARGS% --shared"

if /I "%TOOL%"=="mingw" goto variant_mingw
rem  Visual C++: the default generator, as build.bat (the newest Visual Studio)
call "%ROOT%\generate-cmake.bat" %GENERATE_ARGS%
if errorlevel 1 goto variant_no_msvc
goto variant_build
:variant_mingw
call "%ROOT%\generate-cmake.bat" %GENERATE_ARGS% --generator "%MINGW_GENERATOR%" -- -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
if errorlevel 1 exit /b 1

:variant_build
set "BUILD_ARGS=release --no-pause --build-dir "%BUILD%""
if "%TEST%"=="1" set "BUILD_ARGS=%BUILD_ARGS% --test"
call "%ROOT%\build.bat" %BUILD_ARGS%
if errorlevel 1 exit /b 1

cmake --install "%BUILD%" --config Release --prefix "%PREFIX%"
if errorlevel 1 exit /b 1
set "BUILT=1"

rem  the archive for a release: one folder named as the archive, in dist-release
set "ARCHIVE=windows-x64-%TOOL%-%KIND%"
set "RELEASE=%ROOT%\dist-release"
set "STAGE=%ROOT%\build\dist\archives"
if exist "%STAGE%\%ARCHIVE%" rmdir /s /q "%STAGE%\%ARCHIVE%"
cmake -E copy_directory "%PREFIX%" "%STAGE%\%ARCHIVE%"
if errorlevel 1 exit /b 1
if not exist "%RELEASE%" mkdir "%RELEASE%"
if exist "%RELEASE%\%ARCHIVE%.zip" del /q "%RELEASE%\%ARCHIVE%.zip"
pushd "%STAGE%"
cmake -E tar cf "%RELEASE%\%ARCHIVE%.zip" --format=zip "%ARCHIVE%"
if errorlevel 1 ( popd & exit /b 1 )
popd
echo [dist] %RELEASE%\%ARCHIVE%.zip
exit /b 0

:variant_no_msvc
if /I not "%WHAT%"=="all" exit /b 1
echo.
echo [dist] Visual C++ could not be used: the msvc packages are skipped.
set "SKIPPED=%SKIPPED% msvc"
exit /b 2

rem ---------------------------------------------------------------------------
rem  :checksums <folder>   SHA256SUMS.txt of the archives of a folder (as sha256sum -c reads it)
rem ---------------------------------------------------------------------------
:checksums
if not exist "%~1\*.zip" exit /b 0
pushd "%~1"
if exist SHA256SUMS.txt del /q SHA256SUMS.txt
for %%f in (*.zip) do cmake -E sha256sum "%%f" >> SHA256SUMS.txt
popd
exit /b 0

:failed
echo.
echo [dist] FAILED.
if "%NO_PAUSE%"=="0" pause
endlocal
exit /b 1

:usage
echo Usage: dist.bat [msvc ^| mingw ^| all] [options]
echo.
echo   msvc / mingw / all  the compilers, all by default (one not installed is skipped)
echo   --test              run the tests of each build
echo   --no-pause          do not wait for a key when something fails
echo.
echo   Result: dist\raven-^<version^>\, with
echo   msvc-static, msvc-shared, mingw-static and mingw-shared, and a zip of each;
echo   and dist-release\windows-x64-^<compiler^>-^<kind^>.zip for a release on GitHub.
exit /b 1
