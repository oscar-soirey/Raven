# Raven SDK Installer (tools/installer). Built when RAVEN_BUILD_INSTALLER is ON.
#
#   raven_installer_core       the engine, the platform layer, the compilers, the integration,
#                              the templates (C++17, no dependency)
#   raven_installer_templates  the templates for C programs (raven_templates.h): Raven Designer
#   raven-new                  makes a project from a template (the New Raven Project menu)
#   raven_installer_pack       makes the payload and the installer
#   raven_installer_stub       the installer program, without its payload
#   test_installer             the tests of the engine (ctest)
#
# The installer itself (one file with everything inside) is made by package.cmake, which builds
# the packages of the SDK first: see README.md.

enable_language(CXX)

set(RAVEN_INSTALLER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(raven_installer_core STATIC
	core/util.cpp core/util.hpp
	core/deflate.cpp core/deflate.hpp
	core/json.cpp core/json.hpp
	core/payload.cpp core/payload.hpp
	core/log.cpp core/log.hpp
	core/manifest.cpp core/manifest.hpp
	core/engine.cpp core/engine.hpp
	platform/platform.hpp platform/platform_posix.cpp platform/platform_win32.cpp
	platform/win32_util.cpp platform/win32_util.hpp
	compilers/compilers.cpp compilers/compilers.hpp
	shell/integration.cpp shell/integration_win32.cpp shell/integration.hpp
	templates/templates.cpp templates/templates.hpp
)
target_include_directories(raven_installer_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(raven_installer_core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
	POSITION_INDEPENDENT_CODE ON FOLDER "tools/installer")
if(WIN32)
	target_compile_definitions(raven_installer_core PUBLIC UNICODE _UNICODE)
	target_link_libraries(raven_installer_core PUBLIC shell32 ole32 advapi32 uuid)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(raven_installer_core PUBLIC -Wall -Wextra)
endif()
if(MSVC)
	target_compile_options(raven_installer_core PUBLIC /utf-8 /EHsc /W3)
	target_compile_definitions(raven_installer_core PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
	target_link_libraries(raven_installer_core PUBLIC stdc++fs)
endif()

# Programs that must start on a machine with nothing installed: no DLL of the compiler beside them.
function(raven_installer_standalone target)
	set_target_properties(${target} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON FOLDER "tools/installer")
	if(MINGW)
		target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND NOT APPLE)
		target_link_options(${target} PRIVATE -static-libgcc -static-libstdc++)
	elseif(MSVC AND NOT RAVEN_MSVC_STATIC_RUNTIME)
		message(WARNING "${target}: configure with -DRAVEN_MSVC_STATIC_RUNTIME=ON, or it needs the Visual C++ redistributable")
	endif()
endfunction()

# the templates, for Raven Designer (C)
add_library(raven_installer_templates STATIC templates/raven_templates.cpp templates/raven_templates.h)
target_link_libraries(raven_installer_templates PUBLIC raven_installer_core)
target_include_directories(raven_installer_templates PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/templates")
set_target_properties(raven_installer_templates PROPERTIES CXX_STANDARD 17 FOLDER "tools/installer")

# raven-new: started by the context menus, without a console on Windows
add_executable(raven-new templates/raven_new.cpp)
target_link_libraries(raven-new PRIVATE raven_installer_core)
raven_installer_standalone(raven-new)
if(MSVC)
	target_link_options(raven-new PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
elseif(MINGW)
	target_link_options(raven-new PRIVATE -mwindows)
endif()

add_executable(raven_installer_pack pack/pack_main.cpp)
target_link_libraries(raven_installer_pack PRIVATE raven_installer_core)
set_target_properties(raven_installer_pack PROPERTIES CXX_STANDARD 17 FOLDER "tools/installer")

# the interface: every file of ui/xml and the logo, embedded
file(GLOB RAVEN_INSTALLER_XML CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui/xml/*.xml")
set(RAVEN_INSTALLER_LOGO "${PROJECT_SOURCE_DIR}/docs/logo-com-192.png")
set(RAVEN_INSTALLER_EMBEDDED ${RAVEN_INSTALLER_XML})
set(_logo "${CMAKE_CURRENT_BINARY_DIR}/logo.png")
configure_file("${RAVEN_INSTALLER_LOGO}" "${_logo}" COPYONLY)
list(APPEND RAVEN_INSTALLER_EMBEDDED "${_logo}")
set(_resources "${CMAKE_CURRENT_BINARY_DIR}/resources.cpp")
add_custom_command(OUTPUT "${_resources}"
	COMMAND "${CMAKE_COMMAND}" "-DOUTPUT=${_resources}" "-DPREFIX=ui" "-DFILES=${RAVEN_INSTALLER_EMBEDDED}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedResources.cmake"
	DEPENDS ${RAVEN_INSTALLER_EMBEDDED} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedResources.cmake"
	COMMENT "Embedding the interface of the installer"
	VERBATIM)

set(RAVEN_INSTALLER_UI_SOURCES ui/main.cpp ui/app.cpp ui/app.hpp ui/cli.cpp ui/cli.hpp ui/session.cpp ui/session.hpp
	ui/resources.hpp "${_resources}" ${RAVEN_INSTALLER_XML})
# the icon, the manifest and the version of the program (OFF: a cross build without a resource compiler)
option(RAVEN_INSTALLER_WIN32_RESOURCES "Windows: compile ui/installer.rc into the installer" ON)
if(WIN32 AND RAVEN_INSTALLER_WIN32_RESOURCES)
	list(APPEND RAVEN_INSTALLER_UI_SOURCES ui/installer.rc)
endif()
add_executable(raven_installer_stub WIN32 ${RAVEN_INSTALLER_UI_SOURCES})
target_include_directories(raven_installer_stub PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ui")
target_link_libraries(raven_installer_stub PRIVATE raven_installer_core raven::raven)
raven_installer_standalone(raven_installer_stub)
source_group("xml" FILES ${RAVEN_INSTALLER_XML})
if(MINGW)
	target_link_options(raven_installer_stub PRIVATE -municode)
elseif(MSVC AND RAVEN_INSTALLER_WIN32_RESOURCES)
	# installer.rc has its own manifest
	target_link_options(raven_installer_stub PRIVATE /MANIFEST:NO)
endif()

if(RAVEN_BUILD_TESTS)
	add_executable(test_installer tests/test_installer.cpp)
	target_link_libraries(test_installer PRIVATE raven_installer_core raven_installer_templates)
	set_target_properties(test_installer PROPERTIES CXX_STANDARD 17 FOLDER "tools/installer")
	target_compile_definitions(test_installer PRIVATE "RAVEN_INSTALLER_TEMPLATES=\"${CMAKE_CURRENT_SOURCE_DIR}/templates/files\"")
	add_test(NAME installer_core COMMAND test_installer "${CMAKE_CURRENT_BINARY_DIR}/test_installer_work")
endif()
