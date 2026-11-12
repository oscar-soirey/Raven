cmake_minimum_required(VERSION 3.16)

project(raven VERSION 0.2.3 DESCRIPTION "Raven - lightweight multithreaded GUI library in C" LANGUAGES C)

if(APPLE)
	# Cocoa is the native macOS window API; almost all Raven code remains C.
	enable_language(OBJC)
	set(CMAKE_OBJC_STANDARD 11)
	set(CMAKE_OBJC_STANDARD_REQUIRED ON)
	add_compile_definitions(_DARWIN_C_SOURCE)
endif()

# ---------------------------------------------------------------------------
#  Options
# ---------------------------------------------------------------------------

if(EMSCRIPTEN)
    set(RAVEN_PLATFORM "emscripten" CACHE STRING "Window system")
    set(RAVEN_BACKEND_VULKAN OFF CACHE BOOL "Vulkan")
    set(RAVEN_BUILD_WEB OFF CACHE BOOL "Native Chromium embedding")
    set(RAVEN_BUILD_TOOLS OFF CACHE BOOL "Desktop tools")
    set(RAVEN_BUILD_TESTS OFF CACHE BOOL "Native tests")
endif()
option(RAVEN_BACKEND_WEBGL "Compile the WebGL 2 backend" ${EMSCRIPTEN})

# ON when Raven is the project being built, OFF when it is added with add_subdirectory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(RAVEN_IS_TOP_LEVEL ON)
else()
	set(RAVEN_IS_TOP_LEVEL OFF)
endif()

option(RAVEN_BUILD_SHARED "Build Raven as a shared library (raven.dll / libraven.so)" OFF)
option(RAVEN_BUILD_EXAMPLES "Build the example programs" ${RAVEN_IS_TOP_LEVEL})
option(RAVEN_BUILD_TESTS "Build the tests and the benchmark" ${RAVEN_IS_TOP_LEVEL})
option(RAVEN_BUILD_TOOLS "Build the tools (the designer)" ${RAVEN_IS_TOP_LEVEL})
option(RAVEN_BUILD_INSTALLER "Build the Raven SDK Installer (tools/installer, C++17)" OFF)
option(RAVEN_BUILD_WEB "Compile the web pages (raven_web.h: they drive an installed Chromium-based browser) into the library" ON)
option(RAVEN_BACKEND_VULKAN "Compile the Vulkan backend" ON)
option(RAVEN_BACKEND_SOKOL "Compile the sokol_gfx backend, selectable with R_BACKEND_SOKOL (docs/SOKOL_BACKEND_PLAN.md)" OFF)
set(RAVEN_SOKOL_API "auto" CACHE STRING "Graphics API of the Sokol backend (fixed when Raven is built): auto, gles3 (Linux) or metal (macOS, iOS)")
set_property(CACHE RAVEN_SOKOL_API PROPERTY STRINGS auto gles3 metal)
option(RAVEN_WITH_STB_IMAGE "Decode PNG / JPEG / BMP / TGA textures with stb_image" ON)
option(RAVEN_WITH_SVG "Draw SVG images (icons) with nanosvg" ON)
option(RAVEN_FETCH_VULKAN_HEADERS "Download the Vulkan headers when no Vulkan SDK is found" ON)
option(RAVEN_REBUILD_SHADERS "Recompile the GLSL shaders (needs glslangValidator or glslc)" OFF)
option(RAVEN_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(RAVEN_MSVC_STATIC_RUNTIME "Visual C++: link the C runtime statically (/MT) instead of using its DLL (/MD)" OFF)
set(RAVEN_PLATFORM "auto" CACHE STRING "Window system: auto, win32, x11, cocoa, uikit (iOS), web (Emscripten, with the Sokol backend), emscripten (Emscripten, with the WebGL backend) or null")
option(RAVEN_WAYLAND "Linux and BSD: compile native Wayland beside X11 (nothing of Wayland is needed to build: its libraries are opened at run time)" ON)
set_property(CACHE RAVEN_PLATFORM PROPERTY STRINGS auto win32 x11 cocoa uikit web emscripten null)

if(RAVEN_IS_TOP_LEVEL)
	# one folder for the programs and the DLL, so that a shared build runs in place
	if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	endif()
	# generators with a single configuration (Makefiles, Ninja): optimized by default
	get_property(RAVEN_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
	if(NOT RAVEN_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug, Release, RelWithDebInfo or MinSizeRel" FORCE)
	endif()
	# Visual Studio: targets sorted in folders, F5 starts an example
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	if(RAVEN_BUILD_EXAMPLES)
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT raven_editor)
	endif()
endif()

if(MSVC AND RAVEN_MSVC_STATIC_RUNTIME)
	# for a program that must run without the Visual C++ redistributable
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(RavenCompilerOptions)

if(EMSCRIPTEN)
    if(RAVEN_BACKEND_VULKAN OR RAVEN_BUILD_WEB OR RAVEN_BUILD_SHARED)
        message(FATAL_ERROR "Browser builds require RAVEN_BACKEND_VULKAN=OFF, RAVEN_BUILD_WEB=OFF and RAVEN_BUILD_SHARED=OFF")
    endif()
endif()

# ---------------------------------------------------------------------------
#  Window system
# ---------------------------------------------------------------------------

if(RAVEN_PLATFORM STREQUAL "auto")
	if(EMSCRIPTEN)
		# the WebGL backend's platform; the Sokol one is chosen with -DRAVEN_PLATFORM=web (and -DRAVEN_BACKEND_WEBGL=OFF)
		set(RAVEN_PLATFORM_SELECTED "emscripten")
	elseif(WIN32)
		set(RAVEN_PLATFORM_SELECTED "win32")
	elseif(IOS)
		set(RAVEN_PLATFORM_SELECTED "uikit")
	elseif(APPLE)
		set(RAVEN_PLATFORM_SELECTED "cocoa")
	elseif(UNIX)
		set(RAVEN_PLATFORM_SELECTED "x11")
	else()
		set(RAVEN_PLATFORM_SELECTED "null")
		message(WARNING "Raven: no native window system is implemented for this OS: building the headless (null) platform only.")
	endif()
else()
	set(RAVEN_PLATFORM_SELECTED "${RAVEN_PLATFORM}")
endif()

# ---------------------------------------------------------------------------
#  Sources
# ---------------------------------------------------------------------------

set(RAVEN_PUBLIC_HEADERS
	include/Raven.h
	include/raven/raven_core.h
	include/raven/raven_window.h
	include/raven/raven_touch.h
	include/raven/raven_style.h
	include/raven/raven_resources.h
	include/raven/raven_widget.h
	include/raven/raven_layout.h
	include/raven/raven_controls.h
	include/raven/raven_text_editor.h
	include/raven/raven_shell.h
	include/raven/raven_dialogs.h
	include/raven/raven_viewport.h
	include/raven/raven_draw.h
	include/raven/raven_xml.h
	include/raven/raven_locale.h
	include/raven/raven_accessibility.h
	include/raven/raven_backend.h
	include/raven/raven_host.h
	include/raven/raven_vulkan.h
)

set(RAVEN_CORE_SOURCES
	src/core/base.h
	src/core/context.c
	src/core/locale.c
	src/core/accessibility.c
	src/core/context.h
	src/core/error.c
	src/core/file.c
	src/core/handle.c
	src/core/handle.h
	src/core/heap.c
	src/core/fs.c
	src/core/fs_watch.c
	src/core/fs_watch.h
	src/core/fs.h
	src/core/heap.h
	src/core/jobs.c
	src/core/jobs.h
	src/core/lfmap.c
	src/core/lfmap.h
	src/core/memory.c
	src/core/thread.h
	src/core/utf8.c
)

set(RAVEN_PLATFORM_SOURCES
	src/platform/platform.c
	src/platform/notify.c
	src/platform/taskbar.c
	src/platform/tray.c
	src/platform/platform.h
	src/platform/null/null_platform.c
	src/platform/host/host_platform.c
)

set(RAVEN_RENDER_SOURCES
	src/render/draw.c
	src/render/draw.h
	src/render/font.c
	src/render/font_system.c
	src/render/bidi.c
	src/render/font.h
	src/render/font_raster.c
	src/render/image_decode.c
	src/render/svg_decode.c
	src/render/renderer.c
	src/render/renderer.h
	src/render/texture.c
	src/render/shader.c
	src/render/rfx.c
	src/render/rfx.h
	src/render/device.c
	src/render/null/null_backend.c
	src/resources/default_font.c
	src/resources/mono_font.c
)

set(RAVEN_WIDGET_SOURCES
	src/widget/actions.c
	src/widget/anim.c
	src/widget/events.c
	src/widget/touch.c
	src/widget/layout.c
	src/widget/direction.c
	src/widget/paint.c
	src/widget/registry.c
	src/widget/style.c
	src/widget/theme.c
	src/widget/look.c
	src/widgets/platform_looks.c
	src/widget/widget.c
	src/widget/widget.h
	src/widget/touch.c
	src/widget/window.c
	src/widget/inspector.c
)

set(RAVEN_WIDGETS_SOURCES
	src/widgets/common.c
	src/widgets/common.h
	src/widgets/textedit.c
	# panels
	src/widgets/background_blur.c
	src/widgets/border.c
	src/widgets/box_panel.c
	src/widgets/canvas.c
	src/widgets/expander.c
	src/widgets/grid.c
	src/widgets/overlay.c
	src/widgets/scroll_box.c
	src/widgets/size_box.c
	src/widgets/splitter.c
	src/widgets/switcher.c
	src/widgets/tab_view.c
	src/widgets/wrap_box.c
	# controls
	src/widgets/button.c
	src/widgets/checkbox.c
	src/widgets/color_picker.c
	src/widgets/timeline.c
	src/widgets/node_graph.c
	src/widgets/canvas_look.c
	src/widgets/block_editor.c
	src/widgets/tile_view.c
	src/widgets/combo_box.c
	src/widgets/image.c
	src/widgets/label.c
	src/widgets/markdown.c
	src/widgets/list_view.c
	src/widgets/menu.c
	src/widgets/progress_bar.c
	src/widgets/slider.c
	src/widgets/spin_box.c
	src/widgets/syntax.c
	src/widgets/text_editor.c
	src/widgets/header_bar.c
	src/widgets/file_browser.c
	src/widgets/dialogs.c
	src/widgets/dock_area.c
	src/widgets/island.c
	src/widgets/text_input.c
	src/widgets/tree_view.c
	src/widgets/viewport.c
	src/widgets/table.c
	src/widgets/date_picker.c
	src/widgets/time_picker.c
	src/widgets/toast.c
	src/widgets/breadcrumb.c
	src/widgets/rating.c
	src/widgets/pagination.c
	src/widgets/plot.c
)

set(RAVEN_XML_SOURCES
	src/xml/xml.h
	src/xml/xml_attributes.c
	src/xml/xml_components.c
	src/xml/xml_dom.c
	src/xml/xml_schema.c
	src/xml/xml_loader.c
	src/xml/xml_watch.c
	src/xml/xml_parser.c
)

set(RAVEN_DEFINITIONS "")
set(RAVEN_LIBRARIES "")

# core/file.c uses CoreFoundation even with the null or host platform.
if(APPLE)
	find_library(RAVEN_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
	list(APPEND RAVEN_LIBRARIES ${RAVEN_COREFOUNDATION_FRAMEWORK})
endif()

# The HTTP(S) image loader uses WinHTTP even in a headless Windows build.
if(WIN32)
	list(APPEND RAVEN_LIBRARIES winhttp)
endif()

# threads
if(WIN32)
	list(APPEND RAVEN_CORE_SOURCES src/core/thread_win32.c)
else()
	list(APPEND RAVEN_CORE_SOURCES src/core/thread_posix.c)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	list(APPEND RAVEN_LIBRARIES Threads::Threads)
	# libm, and libdl for loading the Vulkan library at run time
	list(APPEND RAVEN_LIBRARIES m ${CMAKE_DL_LIBS})
endif()
if(APPLE)
	# the system language (core/locale.c) comes from CoreFoundation, with any platform: the Cocoa one pulls it in on macOS,
	# the null platform (headless runs, iOS for now) does not
	find_library(RAVEN_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
	list(APPEND RAVEN_LIBRARIES ${RAVEN_COREFOUNDATION_FRAMEWORK})
endif()

# window system
if(RAVEN_PLATFORM_SELECTED STREQUAL "win32")
	list(APPEND RAVEN_PLATFORM_SOURCES
		src/platform/win32/win32_platform.c
		src/platform/win32/win32_uia.c
		src/platform/win32/win32_manipulation.c
		src/platform/win32/win32_taskbar.c
		src/platform/win32/win32_taskbar.h
		src/platform/win32/win32_manipulation.h
	)
	list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_WIN32)
	list(APPEND RAVEN_LIBRARIES user32 gdi32 shell32 comdlg32 ole32)
	if(MINGW)
		# some MinGW distributions declare the API of Windows XP unless told otherwise; Raven needs Windows 7
		list(APPEND RAVEN_DEFINITIONS _WIN32_WINNT=0x0601 WINVER=0x0601)
	endif()
elseif(RAVEN_PLATFORM_SELECTED STREQUAL "x11")
	find_package(X11 REQUIRED)
	list(APPEND RAVEN_PLATFORM_SOURCES
		src/platform/x11/x11_platform.c
		src/platform/atspi.c
		src/platform/x11/x11_keys.c
		src/platform/x11/x11_xi2.c
		src/platform/x11/x11_internal.h
		src/platform/sni.c
		src/platform/sni.h
	)
	list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_X11)
	list(APPEND RAVEN_LIBRARIES ${X11_X11_LIB})
	if(RAVEN_WAYLAND)
		list(APPEND RAVEN_PLATFORM_SOURCES
			src/platform/wayland/wayland_platform.c
			src/platform/wayland/wayland_protocols.h
		)
		list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_WAYLAND)
	endif()
elseif(RAVEN_PLATFORM_SELECTED STREQUAL "cocoa")
	if(NOT APPLE)
		message(FATAL_ERROR "Raven: the cocoa platform is only available on macOS")
	endif()
	list(APPEND RAVEN_PLATFORM_SOURCES src/platform/cocoa/cocoa_platform.m)
	list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_COCOA)
	find_library(RAVEN_APPKIT_FRAMEWORK AppKit)
	find_library(RAVEN_QUARTZCORE_FRAMEWORK QuartzCore)
	find_library(RAVEN_METAL_FRAMEWORK Metal)
	if(NOT RAVEN_APPKIT_FRAMEWORK OR NOT RAVEN_QUARTZCORE_FRAMEWORK OR NOT RAVEN_METAL_FRAMEWORK)
		message(FATAL_ERROR "Raven: Cocoa builds require the AppKit, QuartzCore and Metal frameworks")
	endif()
	list(APPEND RAVEN_LIBRARIES ${RAVEN_APPKIT_FRAMEWORK} ${RAVEN_QUARTZCORE_FRAMEWORK} ${RAVEN_METAL_FRAMEWORK})
elseif(RAVEN_PLATFORM_SELECTED STREQUAL "uikit")
	if(NOT IOS)
		message(FATAL_ERROR "Raven: the uikit platform is for iOS (-DCMAKE_SYSTEM_NAME=iOS)")
	endif()
	list(APPEND RAVEN_PLATFORM_SOURCES src/platform/uikit/uikit_platform.m)
	list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_UIKIT)
	find_library(RAVEN_UIKIT_FRAMEWORK UIKit)
	find_library(RAVEN_QUARTZCORE_FRAMEWORK QuartzCore)
	find_library(RAVEN_COREGRAPHICS_FRAMEWORK CoreGraphics)
	if(NOT RAVEN_UIKIT_FRAMEWORK OR NOT RAVEN_QUARTZCORE_FRAMEWORK OR NOT RAVEN_COREGRAPHICS_FRAMEWORK)
		message(FATAL_ERROR "Raven: iOS builds require the UIKit, QuartzCore and CoreGraphics frameworks")
	endif()
	list(APPEND RAVEN_LIBRARIES ${RAVEN_UIKIT_FRAMEWORK} ${RAVEN_QUARTZCORE_FRAMEWORK} ${RAVEN_COREGRAPHICS_FRAMEWORK})
elseif(RAVEN_PLATFORM_SELECTED STREQUAL "web")
	if(NOT EMSCRIPTEN)
		message(FATAL_ERROR "Raven: the web platform needs Emscripten (emcmake cmake ...)")
	endif()
	list(APPEND RAVEN_PLATFORM_SOURCES src/platform/web/web_platform.c)
	# EM_JS needs the GNU dialect of C
	set_source_files_properties(src/platform/web/web_platform.c PROPERTIES COMPILE_OPTIONS "-std=gnu11")
	list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_WEB)
elseif(RAVEN_PLATFORM_SELECTED STREQUAL "emscripten")
	if(NOT EMSCRIPTEN)
		message(FATAL_ERROR "The emscripten platform requires the Emscripten toolchain")
	endif()
	list(APPEND RAVEN_PLATFORM_SOURCES src/platform/emscripten/emscripten_platform.c
		src/platform/emscripten/emscripten_services.c
		src/platform/emscripten/emscripten_accessibility.c)
	list(APPEND RAVEN_DEFINITIONS RAVEN_PLATFORM_EMSCRIPTEN)
elseif(NOT RAVEN_PLATFORM_SELECTED STREQUAL "null")
	message(FATAL_ERROR "Raven: unknown RAVEN_PLATFORM '${RAVEN_PLATFORM_SELECTED}' (expected auto, win32, x11, cocoa, uikit, web, emscripten or null)")
endif()

if(RAVEN_BACKEND_WEBGL)
	if(NOT EMSCRIPTEN)
		message(FATAL_ERROR "The WebGL backend requires Emscripten")
	endif()
	list(APPEND RAVEN_RENDER_SOURCES src/render/webgl/webgl_backend.c)
	list(APPEND RAVEN_DEFINITIONS RAVEN_BACKEND_WEBGL_ENABLED)
endif()

# Vulkan backend: only the headers are needed, the library is loaded at run time
if(RAVEN_BACKEND_VULKAN)
	include(RavenVulkanHeaders)
	list(APPEND RAVEN_RENDER_SOURCES
		src/render/vulkan/vk_backend.c
		src/render/vulkan/vk_backend.h
		src/render/vulkan/vk_device.c
		src/render/vulkan/vk_frame.c
		src/render/vulkan/vk_glass.c
		src/render/vulkan/vk_loader.c
		src/render/vulkan/vk_loader.h
		src/render/vulkan/vk_pipeline.c
		src/render/vulkan/vk_public.c
		src/render/vulkan/vk_surface.c
		src/render/vulkan/vk_texture.c
	)
	list(APPEND RAVEN_DEFINITIONS RAVEN_BACKEND_VULKAN_ENABLED)
	if(RAVEN_REBUILD_SHADERS)
		include(RavenShaders)
		raven_compile_shader(src/render/vulkan/shaders/ui.vert src/render/vulkan/generated/ui_vert_spv.h rvn_ui_vert_spv)
		raven_compile_shader(src/render/vulkan/shaders/ui.frag src/render/vulkan/generated/ui_frag_spv.h rvn_ui_frag_spv)
		raven_compile_shader(src/render/vulkan/shaders/blur.vert src/render/vulkan/generated/blur_vert_spv.h rvn_blur_vert_spv)
		raven_compile_shader(src/render/vulkan/shaders/blur.frag src/render/vulkan/generated/blur_frag_spv.h rvn_blur_frag_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/aurora.frag src/render/vulkan/generated/effect_aurora_spv.h rvn_effect_aurora_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/shimmer.frag src/render/vulkan/generated/effect_shimmer_spv.h rvn_effect_shimmer_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/neon.frag src/render/vulkan/generated/effect_neon_spv.h rvn_effect_neon_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/ripple.frag src/render/vulkan/generated/effect_ripple_spv.h rvn_effect_ripple_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/plasma.frag src/render/vulkan/generated/effect_plasma_spv.h rvn_effect_plasma_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/grain.frag src/render/vulkan/generated/effect_grain_spv.h rvn_effect_grain_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/spotlight.frag src/render/vulkan/generated/effect_spotlight_spv.h rvn_effect_spotlight_spv)
		raven_compile_shader(src/render/vulkan/shaders/effects/mesh.frag src/render/vulkan/generated/effect_mesh_spv.h rvn_effect_mesh_spv)
	endif()
endif()

# Sokol backend: sokol_gfx.h is compiled into the library (third_party/sokol); the graphics API is chosen here
if(RAVEN_BACKEND_SOKOL)
	set(RAVEN_SOKOL_API_SELECTED "${RAVEN_SOKOL_API}")
	if(RAVEN_SOKOL_API_SELECTED STREQUAL "auto")
		if(APPLE)
			set(RAVEN_SOKOL_API_SELECTED "metal")
		elseif(EMSCRIPTEN)
			set(RAVEN_SOKOL_API_SELECTED "gles3")
		elseif(WIN32)
			message(FATAL_ERROR "Raven: the Sokol backend has no graphics API for this platform yet (docs/SOKOL_BACKEND_PLAN.md)")
		else()
			set(RAVEN_SOKOL_API_SELECTED "gles3")
		endif()
	endif()
	list(APPEND RAVEN_RENDER_SOURCES
		src/render/sokol/sg_backend.c
		src/render/sokol/sg_backend.h
		src/render/sokol/sg_glass.c
		src/render/sokol/sg_pipeline.c
		src/render/sokol/sg_prefix.h
		src/render/sokol/sg_texture.c
	)
	if(RAVEN_SOKOL_API_SELECTED STREQUAL "gles3" AND EMSCRIPTEN)
		# WebGL 2 through the GLES3 headers of Emscripten, context of the canvas (sg_web.c)
		list(APPEND RAVEN_RENDER_SOURCES src/render/sokol/sg_web.c src/render/sokol/sg_gl.c src/render/sokol/sg_impl.c)
		list(APPEND RAVEN_DEFINITIONS RAVEN_SOKOL_GLES3)
		set(RAVEN_WEBGL2_LINK_OPTIONS -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2)
	elseif(RAVEN_SOKOL_API_SELECTED STREQUAL "gles3")
		find_path(RAVEN_EGL_INCLUDE_DIR EGL/egl.h)
		find_path(RAVEN_GLES_INCLUDE_DIR GLES3/gl3.h)
		find_library(RAVEN_EGL_LIBRARY EGL)
		find_library(RAVEN_GLES_LIBRARY NAMES GLESv2 GLESv3)
		if(NOT RAVEN_EGL_INCLUDE_DIR OR NOT RAVEN_GLES_INCLUDE_DIR OR NOT RAVEN_EGL_LIBRARY OR NOT RAVEN_GLES_LIBRARY)
			message(FATAL_ERROR "Raven: the Sokol backend (gles3) needs the EGL and GLESv2 development files (Debian: libegl-dev libgles-dev; Fedora: mesa-libEGL-devel mesa-libGLES-devel)")
		endif()
		list(APPEND RAVEN_RENDER_SOURCES src/render/sokol/sg_egl.c src/render/sokol/sg_gl.c src/render/sokol/sg_impl.c)
		list(APPEND RAVEN_DEFINITIONS RAVEN_SOKOL_GLES3)
		list(APPEND RAVEN_LIBRARIES ${RAVEN_EGL_LIBRARY} ${RAVEN_GLES_LIBRARY})
	elseif(RAVEN_SOKOL_API_SELECTED STREQUAL "metal")
		if(NOT APPLE)
			message(FATAL_ERROR "Raven: RAVEN_SOKOL_API=metal is only available on macOS and iOS")
		endif()
		find_library(RAVEN_FOUNDATION_FRAMEWORK Foundation)
		find_library(RAVEN_METAL_FRAMEWORK Metal)
		find_library(RAVEN_QUARTZCORE_FRAMEWORK QuartzCore)
		if(NOT RAVEN_FOUNDATION_FRAMEWORK OR NOT RAVEN_METAL_FRAMEWORK OR NOT RAVEN_QUARTZCORE_FRAMEWORK)
			message(FATAL_ERROR "Raven: the Sokol backend (metal) needs the Foundation, Metal and QuartzCore frameworks")
		endif()
		# sokol_gfx with Metal is Objective-C; both files are written for ARC
		list(APPEND RAVEN_RENDER_SOURCES src/render/sokol/sg_metal.m src/render/sokol/sg_impl.m)
		set_source_files_properties(src/render/sokol/sg_metal.m src/render/sokol/sg_impl.m PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
		list(APPEND RAVEN_DEFINITIONS RAVEN_SOKOL_METAL)
		list(APPEND RAVEN_LIBRARIES ${RAVEN_FOUNDATION_FRAMEWORK} ${RAVEN_METAL_FRAMEWORK} ${RAVEN_QUARTZCORE_FRAMEWORK})
	else()
		message(FATAL_ERROR "Raven: unknown RAVEN_SOKOL_API '${RAVEN_SOKOL_API_SELECTED}' (expected auto, gles3 or metal)")
	endif()
	list(APPEND RAVEN_DEFINITIONS RAVEN_BACKEND_SOKOL_ENABLED)
endif()

if(RAVEN_WITH_SVG)
	list(APPEND RAVEN_DEFINITIONS RAVEN_WITH_SVG)
endif()
if(RAVEN_WITH_STB_IMAGE)
	list(APPEND RAVEN_DEFINITIONS RAVEN_WITH_STB_IMAGE)
endif()

# ---------------------------------------------------------------------------
#  Library
# ---------------------------------------------------------------------------

if(RAVEN_BUILD_SHARED)
	add_library(raven SHARED)
	target_compile_definitions(raven PRIVATE RAVEN_BUILD_DLL INTERFACE RAVEN_USE_DLL)
	set_target_properties(raven PROPERTIES C_VISIBILITY_PRESET hidden)
else()
	add_library(raven STATIC)
endif()
add_library(raven::raven ALIAS raven)
# RAVEN_REBUILD_SHADERS: the headers made from the GLSL shaders are built before the library
get_property(RAVEN_SHADER_HEADERS GLOBAL PROPERTY RAVEN_SHADER_HEADERS)
if(RAVEN_SHADER_HEADERS)
	target_sources(raven PRIVATE ${RAVEN_SHADER_HEADERS})
endif()

# web pages (raven_web.h): compiled into the same library
set(RAVEN_WEB_SOURCES "")
if(RAVEN_BUILD_WEB)
	set(RAVEN_WEB_SOURCES
		web/include/raven_web.h
		web/src/web_internal.h
		web/src/web_json.c
		web/src/web_browser.c
		web/src/web_view.c
		web/src/web_posix.c
		web/src/web_win32.c
	)
endif()

target_sources(raven PRIVATE
	${RAVEN_WEB_SOURCES}
	${RAVEN_PUBLIC_HEADERS}
	${RAVEN_CORE_SOURCES}
	${RAVEN_PLATFORM_SOURCES}
	${RAVEN_RENDER_SOURCES}
	${RAVEN_WIDGET_SOURCES}
	${RAVEN_WIDGETS_SOURCES}
	${RAVEN_XML_SOURCES}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
	${RAVEN_PUBLIC_HEADERS} ${RAVEN_CORE_SOURCES} ${RAVEN_PLATFORM_SOURCES} ${RAVEN_RENDER_SOURCES}
	${RAVEN_WIDGET_SOURCES} ${RAVEN_WIDGETS_SOURCES} ${RAVEN_XML_SOURCES})

target_include_directories(raven
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/web/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
target_compile_definitions(raven PRIVATE ${RAVEN_DEFINITIONS} RVN_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(raven PRIVATE ${RAVEN_LIBRARIES})
set_target_properties(raven PROPERTIES
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
	POSITION_INDEPENDENT_CODE ON
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)
raven_set_compiler_options(raven)
if(EMSCRIPTEN)
    set_target_properties(raven PROPERTIES C_EXTENSIONS ON)
    target_compile_options(raven PRIVATE -Wno-overlength-strings -Wno-dollar-in-identifier-extension
        -Wno-variadic-macro-arguments-omitted
        -Wno-gnu-zero-variadic-macro-arguments)
    target_compile_definitions(raven PRIVATE _POSIX_C_SOURCE=200809L)
    target_link_options(raven PUBLIC
        "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2"
        "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']" "-lidbfs.js" "-sFETCH=1" "-sALLOW_MEMORY_GROWTH=1" "-sENVIRONMENT=web"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','UTF8ToString']")
endif()

if(MINGW AND RAVEN_BUILD_SHARED)
	# the DLL is raven.dll, as with Visual C++ (libraven.dll.a is what programs link with),
	# and it does not need the DLLs of the compiler to be shipped beside it
	set_target_properties(raven PROPERTIES PREFIX "")
	target_link_options(raven PRIVATE -static-libgcc)
endif()

if(RAVEN_PLATFORM_SELECTED STREQUAL "x11")
	target_include_directories(raven PRIVATE ${X11_INCLUDE_DIR})
endif()
if(RAVEN_BACKEND_VULKAN)
	target_include_directories(raven PRIVATE ${RAVEN_VULKAN_INCLUDE_DIR})
endif()
if(RAVEN_WEBGL2_LINK_OPTIONS)
	target_link_options(raven INTERFACE ${RAVEN_WEBGL2_LINK_OPTIONS})
endif()
if(RAVEN_BACKEND_SOKOL AND RAVEN_SOKOL_API_SELECTED STREQUAL "gles3" AND NOT EMSCRIPTEN)
	target_include_directories(raven PRIVATE ${RAVEN_EGL_INCLUDE_DIR} ${RAVEN_GLES_INCLUDE_DIR})
endif()

if(RAVEN_PLATFORM_SELECTED STREQUAL "x11" AND RAVEN_WAYLAND)
	set(RAVEN_PLATFORM_SUMMARY "x11+wayland")
else()
	set(RAVEN_PLATFORM_SUMMARY "${RAVEN_PLATFORM_SELECTED}")
endif()
message(STATUS "Raven ${PROJECT_VERSION}: platform=${RAVEN_PLATFORM_SUMMARY} vulkan=${RAVEN_BACKEND_VULKAN} sokol=${RAVEN_BACKEND_SOKOL} webgl=${RAVEN_BACKEND_WEBGL} shared=${RAVEN_BUILD_SHARED}")

# ---------------------------------------------------------------------------
#  Examples, tools and tests
# ---------------------------------------------------------------------------

if(RAVEN_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
	add_subdirectory(bindings)
endif()

# (before the examples: one of them shows web pages; after enable_testing: it has a test)
include(GNUInstallDirs)
if(RAVEN_BUILD_WEB)
	add_subdirectory(web)
endif()

if(RAVEN_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# (after enable_testing: the designer has a test of its own)
if(RAVEN_BUILD_TOOLS)
	add_subdirectory(tools)
endif()

# the installer of the SDK (its own CMakeLists: it is C++)
if(RAVEN_BUILD_INSTALLER)
	add_subdirectory(tools/installer)
endif()

# ---------------------------------------------------------------------------
#  Install:  find_package(raven)  +  target_link_libraries(app PRIVATE raven::raven)
# ---------------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS raven EXPORT ravenTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/raven DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES include/Raven.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ravenTargets NAMESPACE raven:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raven)

configure_package_config_file(cmake/ravenConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/ravenConfig.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raven)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/ravenConfigVersion.cmake"
	VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ravenConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/ravenConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raven)

# ---------------------------------------------------------------------------
#  Packages:  cmake --build <dir> --target package
#  (dist.bat and dist.sh build the complete ones: static and shared together)
# ---------------------------------------------------------------------------

if(RAVEN_IS_TOP_LEVEL)
	install(FILES README.md THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DATADIR}/raven)
	install(DIRECTORY docs DESTINATION ${CMAKE_INSTALL_DATADIR}/raven)
	# the licenses of what the library contains (THIRD_PARTY.md says what and why)
	file(GLOB RAVEN_THIRD_PARTY_LICENSES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/third_party" "third_party/*/LICENSE*" "third_party/*/OFL*")
	foreach(license ${RAVEN_THIRD_PARTY_LICENSES})
		get_filename_component(component "${license}" DIRECTORY)
		install(FILES "third_party/${license}" DESTINATION ${CMAKE_INSTALL_DATADIR}/raven/licenses/${component})
	endforeach()
	install(DIRECTORY bindings DESTINATION ${CMAKE_INSTALL_DATADIR}/raven
		PATTERN "CMakeLists.txt" EXCLUDE PATTERN "target" EXCLUDE PATTERN "__pycache__" EXCLUDE PATTERN "Cargo.lock" EXCLUDE)

	if(MSVC)
		set(RAVEN_PACKAGE_COMPILER "msvc")
	elseif(MINGW)
		set(RAVEN_PACKAGE_COMPILER "mingw")
	else()
		string(TOLOWER "${CMAKE_C_COMPILER_ID}" RAVEN_PACKAGE_COMPILER)
	endif()
	string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" RAVEN_PACKAGE_SYSTEM)
	if(RAVEN_BUILD_SHARED)
		set(RAVEN_PACKAGE_KIND "shared")
	else()
		set(RAVEN_PACKAGE_KIND "static")
	endif()
	set(CPACK_PACKAGE_NAME "raven")
	set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
	set(CPACK_PACKAGE_FILE_NAME "raven-${PROJECT_VERSION}-${RAVEN_PACKAGE_SYSTEM}-${RAVEN_PACKAGE_COMPILER}-${RAVEN_PACKAGE_KIND}")
	if(WIN32)
		set(CPACK_GENERATOR "ZIP")
	else()
		set(CPACK_GENERATOR "TGZ")
	endif()
	include(CPack)
endif()

# (the browser fixtures and the demo use raven_emscripten.h: the platform of the WebGL backend, not the Sokol web one)
if(EMSCRIPTEN AND RAVEN_PLATFORM_SELECTED STREQUAL "emscripten")
    option(RAVEN_BUILD_WEB_TESTS "Build browser integration fixtures" OFF)
    if(RAVEN_BUILD_WEB_TESTS)
        if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
            set(RAVEN_WEB_TEST_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        else()
            set(RAVEN_WEB_TEST_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bin")
        endif()
        add_executable(raven_web_test tests/web/app.c)
        target_link_libraries(raven_web_test PRIVATE raven::raven)
        set_target_properties(raven_web_test PROPERTIES SUFFIX ".html"
            RUNTIME_OUTPUT_DIRECTORY "${RAVEN_WEB_TEST_OUTPUT}")
        configure_file(tests/web/asset.svg "${RAVEN_WEB_TEST_OUTPUT}/asset.svg" COPYONLY)
        raven_set_compiler_options(raven_web_test)
        target_link_options(raven_web_test PRIVATE
            "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html")
        set_property(TARGET raven_web_test APPEND PROPERTY LINK_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html")
    endif()
    option(RAVEN_BUILD_WEB_EXAMPLE "Build the standalone browser example" ${RAVEN_IS_TOP_LEVEL})
    if(RAVEN_BUILD_WEB_EXAMPLE)
        add_executable(raven_web_demo examples/web/main.c)
        target_link_libraries(raven_web_demo PRIVATE raven::raven)
        raven_set_compiler_options(raven_web_demo)
        set_target_properties(raven_web_demo PROPERTIES SUFFIX ".html")
        target_link_options(raven_web_demo PRIVATE
            "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html")
        set_property(TARGET raven_web_demo APPEND PROPERTY LINK_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html")
    endif()
endif()
