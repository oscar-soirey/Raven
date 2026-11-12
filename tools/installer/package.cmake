# Raven SDK Installer - makes the installer of this platform, from the sources:
#
#   cmake -P tools/installer/package.cmake                      (from the root of the repository)
#   cmake -DFAMILIES=msvc -P tools/installer/package.cmake      (Windows: only Visual C++)
#   cmake -DMOLTENVK=/path/libMoltenVK.dylib -P tools/installer/package.cmake   (macOS)
#
# 1. builds and installs the library packages of the SDK, static and shared, one per
#    compiler of the platform (the folders dist.bat makes): build/installer/packages/<id>
#      Windows  msvc-static msvc-shared (Visual Studio, x64)   mingw-static mingw-shared (gcc)
#      Linux    gnu-static gnu-shared
#      macOS    apple-static apple-shared (universal: arm64 + x86_64)
#    Raven Designer comes from the first static package;
# 2. builds the installer (static C/C++ runtime: it starts on a machine with nothing installed);
# 3. packs: dist-installer/RavenSDKInstaller.exe, dist-installer/RavenSDKInstaller (Linux),
#    dist-installer/RavenSDKInstaller.app (macOS, with Raven Designer.app inside its payload).
#
# Options: -DFAMILIES=a;b  -DBUILD_ROOT=<folder> (default build/installer)  -DGENERATOR=<CMake generator>
#          -DVULKAN_HEADERS=<include folder>  -DJOBS=<n>  -DSKIP_PACKAGES=ON (reuse the packages)

cmake_minimum_required(VERSION 3.16)

get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
if(NOT BUILD_ROOT)
	set(BUILD_ROOT "${ROOT}/build/installer")
endif()
set(OUT "${ROOT}/dist-installer")
if(NOT JOBS)
	cmake_host_system_information(RESULT JOBS QUERY NUMBER_OF_LOGICAL_CORES)
endif()

file(STRINGS "${ROOT}/CMakeLists.txt" _project REGEX "^project\\(raven VERSION ")
string(REGEX MATCH "VERSION ([0-9.]+)" _ "${_project}")
set(VERSION "${CMAKE_MATCH_1}")

if(CMAKE_HOST_WIN32)
	set(PLATFORM "windows-x64")
	set(EXE ".exe")
	if(NOT FAMILIES)
		set(FAMILIES msvc)
		find_program(_gcc gcc)
		if(_gcc)
			list(APPEND FAMILIES mingw)
		endif()
	endif()
elseif(CMAKE_HOST_APPLE)
	set(PLATFORM "macos-universal")
	set(EXE "")
	set(FAMILIES apple)
else()
	set(PLATFORM "linux-x64")
	set(EXE "")
	set(FAMILIES gnu)
endif()

if(NOT GENERATOR)
	find_program(_ninja ninja)
	if(_ninja)
		set(GENERATOR "Ninja")
	elseif(NOT CMAKE_HOST_WIN32)
		set(GENERATOR "Unix Makefiles")
	endif()
endif()

set(COMMON_ARGS -DRAVEN_BUILD_EXAMPLES=OFF -DRAVEN_BUILD_TESTS=OFF)
if(VULKAN_HEADERS)
	list(APPEND COMMON_ARGS "-DRAVEN_VULKAN_INCLUDE_DIR=${VULKAN_HEADERS}")
endif()
if(CMAKE_HOST_APPLE)
	list(APPEND COMMON_ARGS "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0)
endif()

function(run)
	execute_process(COMMAND ${ARGN} RESULT_VARIABLE _result)
	if(NOT _result EQUAL 0)
		string(REPLACE ";" " " _line "${ARGN}")
		message(FATAL_ERROR "failed (${_result}): ${_line}")
	endif()
endfunction()

# The generator and compiler arguments of a family
function(family_args family out)
	set(args "")
	if(family STREQUAL "msvc")
		# the Visual Studio generator: the newest Visual Studio, x64
		list(APPEND args -A x64)
	elseif(family STREQUAL "mingw")
		list(APPEND args -G "${GENERATOR}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++)
		if(NOT GENERATOR)
			set(args -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++)
		endif()
	else()
		list(APPEND args -G "${GENERATOR}" -DCMAKE_BUILD_TYPE=Release)
	endif()
	set(${out} ${args} PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
#  1. The packages
# ---------------------------------------------------------------------------

set(TARGET_ARGS "")
set(DESIGNER_PREFIX "")
foreach(family IN LISTS FAMILIES)
	foreach(kind static shared)
		set(id "${family}-${kind}")
		set(build "${BUILD_ROOT}/build-${id}")
		set(prefix "${BUILD_ROOT}/packages/${id}")
		family_args(${family} args)
		set(shared OFF)
		if(kind STREQUAL "shared")
			set(shared ON)
		endif()
		# the designer is built once, with the first static library
		set(tools OFF)
		if(kind STREQUAL "static" AND NOT DESIGNER_PREFIX)
			set(tools ON)
			set(DESIGNER_PREFIX "${prefix}")
		endif()
		if(NOT SKIP_PACKAGES)
			message(STATUS "=== package ${id}")
			run("${CMAKE_COMMAND}" -S "${ROOT}" -B "${build}" ${args} ${COMMON_ARGS} -DRAVEN_BUILD_SHARED=${shared} -DRAVEN_BUILD_TOOLS=${tools})
			run("${CMAKE_COMMAND}" --build "${build}" --config Release --parallel ${JOBS})
			file(REMOVE_RECURSE "${prefix}")
			run("${CMAKE_COMMAND}" --install "${build}" --config Release --prefix "${prefix}")
			# the tools are the Raven Designer component, not part of the packages
			file(GLOB _tools "${prefix}/bin/raven_designer*" "${prefix}/bin/raven_theme_editor*")
			if(tools)
				file(REMOVE_RECURSE "${BUILD_ROOT}/designer")
				file(COPY "${prefix}/bin/raven_designer${EXE}" DESTINATION "${BUILD_ROOT}/designer")
			endif()
			if(_tools)
				file(REMOVE ${_tools})
			endif()
			file(COPY "${ROOT}/LICENSE" "${ROOT}/NOTICE" DESTINATION "${prefix}/share/raven")
		endif()
		list(APPEND TARGET_ARGS --target "${id}=${prefix}")
	endforeach()
endforeach()

# ---------------------------------------------------------------------------
#  2. The installer
# ---------------------------------------------------------------------------

set(tools_build "${BUILD_ROOT}/build-installer")
list(GET FAMILIES 0 first_family)
family_args(${first_family} args)
message(STATUS "=== installer")
run("${CMAKE_COMMAND}" -S "${ROOT}" -B "${tools_build}" ${args} ${COMMON_ARGS} -DRAVEN_BUILD_SHARED=OFF -DRAVEN_BUILD_TOOLS=OFF
	-DRAVEN_BUILD_WEB=OFF -DRAVEN_BUILD_INSTALLER=ON -DRAVEN_MSVC_STATIC_RUNTIME=ON)
run("${CMAKE_COMMAND}" --build "${tools_build}" --config Release --parallel ${JOBS}
	--target raven_installer_stub raven-new raven_installer_pack)
# the programs: bin/ (Ninja, Makefiles) or bin/Release (Visual Studio)
set(bin "${tools_build}/bin")
if(EXISTS "${tools_build}/bin/Release/raven_installer_stub${EXE}")
	set(bin "${tools_build}/bin/Release")
endif()

# Raven Designer
set(_designer "${BUILD_ROOT}/designer/raven_designer${EXE}")
if(NOT EXISTS "${_designer}")
	message(FATAL_ERROR "Raven Designer was not built (${_designer})")
endif()

# ---------------------------------------------------------------------------
#  3. Packing
# ---------------------------------------------------------------------------

file(MAKE_DIRECTORY "${OUT}")
set(PACK_ARGS --version "${VERSION}" --platform "${PLATFORM}" ${TARGET_ARGS}
	--templates "${ROOT}/tools/installer/templates/files"
	--sdk-bin-file "raven-new${EXE}=${bin}/raven-new${EXE}")

if(CMAKE_HOST_APPLE)
	# Raven Designer.app and RavenSDKInstaller.app, each with MoltenVK (Raven draws with Vulkan)
	if(NOT MOLTENVK)
		foreach(_candidate /opt/homebrew/lib/libMoltenVK.dylib /usr/local/lib/libMoltenVK.dylib)
			if(EXISTS "${_candidate}")
				set(MOLTENVK "${_candidate}")
			endif()
		endforeach()
	endif()
	if(NOT MOLTENVK)
		message(FATAL_ERROR "macOS: give -DMOLTENVK=<libMoltenVK.dylib> (brew install molten-vk, or the Vulkan SDK)")
	endif()
	function(make_bundle bundle executable name identifier)
		file(REMOVE_RECURSE "${bundle}")
		file(MAKE_DIRECTORY "${bundle}/Contents/MacOS" "${bundle}/Contents/Resources" "${bundle}/Contents/Frameworks")
		file(COPY "${executable}" DESTINATION "${bundle}/Contents/MacOS")
		file(COPY "${MOLTENVK}" DESTINATION "${bundle}/Contents/Frameworks" FOLLOW_SYMLINK_CHAIN)
		get_filename_component(_exe_name "${executable}" NAME)
		file(WRITE "${bundle}/Contents/Info.plist" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
	<key>CFBundleName</key><string>${name}</string>
	<key>CFBundleDisplayName</key><string>${name}</string>
	<key>CFBundleIdentifier</key><string>${identifier}</string>
	<key>CFBundleExecutable</key><string>${_exe_name}</string>
	<key>CFBundlePackageType</key><string>APPL</string>
	<key>CFBundleShortVersionString</key><string>${VERSION}</string>
	<key>CFBundleVersion</key><string>${VERSION}</string>
	<key>LSMinimumSystemVersion</key><string>11.0</string>
	<key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
")
	endfunction()
	set(designer_app "${BUILD_ROOT}/Raven Designer.app")
	make_bundle("${designer_app}" "${_designer}" "Raven Designer" "io.github.oscar-soirey.raven.designer")
	set(installer_app "${OUT}/RavenSDKInstaller.app")
	make_bundle("${installer_app}" "${bin}/raven_installer_stub" "Raven SDK Installer" "io.github.oscar-soirey.raven.sdkinstaller")
	file(RENAME "${installer_app}/Contents/MacOS/raven_installer_stub" "${installer_app}/Contents/MacOS/RavenSDKInstaller")
	file(READ "${installer_app}/Contents/Info.plist" _plist)
	string(REPLACE "<string>raven_installer_stub</string>" "<string>RavenSDKInstaller</string>" _plist "${_plist}")
	file(WRITE "${installer_app}/Contents/Info.plist" "${_plist}")
	run("${bin}/raven_installer_pack" ${PACK_ARGS} --designer "${designer_app}" --designer-launch "Raven Designer.app"
		--out "${installer_app}/Contents/Resources/payload.rvnpk")
	# an ad-hoc signature: Apple silicon runs nothing unsigned (a release is signed with a Developer ID, then notarized)
	execute_process(COMMAND codesign --force --deep --sign - "${installer_app}")
	# the bundle in one file, for a download (ditto keeps the attributes of the bundle)
	file(REMOVE "${OUT}/RavenSDKInstaller-macos.zip")
	run(ditto -c -k --keepParent "${installer_app}" "${OUT}/RavenSDKInstaller-macos.zip")
	message(STATUS "=== ${installer_app}")
else()
	set(icon "${ROOT}/docs/logo-com-192.png")
	if(CMAKE_HOST_WIN32)
		set(icon "${ROOT}/tools/designer/logo-com.ico")
	endif()
	set(installer "${OUT}/RavenSDKInstaller${EXE}")
	run("${bin}/raven_installer_pack" ${PACK_ARGS} --designer-file "raven_designer${EXE}=${_designer}" --designer-icon "${icon}"
		--stub "${bin}/raven_installer_stub${EXE}" --out "${installer}")
	file(SHA256 "${installer}" _sha)
	file(WRITE "${installer}.sha256" "${_sha}  RavenSDKInstaller${EXE}\n")
	message(STATUS "=== ${installer}")
endif()
