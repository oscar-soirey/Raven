# Raven examples.
#
#   hello            a window built with the C API
#   editor           a window described in XML, software-rendered viewport,
#                    custom widget, virtual list
#   glass            the glass themes: translucent, refracting controls and panels
#   notepad          a text editor: custom header bar, languages, file dialogs,
#                    files dropped from the system
#   animations       transitions, springs in a custom widget, an island
#   workspace        panels that dock, a file browser, materials
#   components       tags declared in XML with <Widget>, documents built from C
#   memory           an allocator given by the application, and the statistics
#   studio           a graph of nodes, a timeline and a color picker; live reload of its XML
#   themes           looks (Raven, Windows 11), theme files, styles of parts and states, Markdown
#   overlay          a window that is seen through, and the four ways to blur what is behind
#   effects          the effects (shaders) of Raven, and two written by the application
#   plot             curves of X / Y data: XML, streaming data, logarithmic axes, drag lines, a million points
#   charts           a dashboard: KPI tiles with sparklines, a pie, a donut, gauges with bands, a heatmap
#   map              a MapView whose tiles it draws itself (offline), markers, flights; or tiles from files
#   carousel         pages swiped one at a time: an onboarding with dots, arrows, loop and autoplay
#   transfer         pictures and formats on the clipboard, swatches dragged out to other applications
#   web_view         a page shown by the web engine of the system (WKWebView), scripts and messages
#   icons            the icons shipped with Raven, in the color of the text: a gallery, buttons, tabs, a tree
#   notifications    notifications of the system with a tag, buttons and an icon; a toast where there are none
#   animated_images  animated GIF and APNG in Image widgets: play, pause, stop, seek, loop count
#   diff_view        two versions of a file compared: side by side or unified, folds, changes, a note
#   browser          web pages in the interface, with the optional library raven_web
#   music            a music player (no sound): an interface in several XML files,
#                    tags made by the application, themes, the accent of the cover, an island
#   ui_test          a test of an interface: widgets driven by name, a golden image
#   headless         scripts that drive hello and music from outside (RAVEN_HEADLESS, RAVEN_SCRIPT)
#   viewport_vulkan  a Vulkan scene shown in the UI (offscreen image + in-pass draw)
#   viewport_opengl  an OpenGL scene shown in a native child window
#   mobile           the iOS / Android showcase in a desktop window, with its Device tab

function(raven_add_example name)
	add_executable(raven_${name} ${ARGN} common/example.h)
	target_link_libraries(raven_${name} PRIVATE raven::raven)
	set_target_properties(raven_${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "examples")
	# where the example finds its data files when started from any folder
	if(EMSCRIPTEN)
		target_compile_definitions(raven_${name} PRIVATE
			RAVEN_EXAMPLE_DIR="/examples/${name}")
		set_target_properties(raven_${name} PROPERTIES SUFFIX ".html")
		target_link_options(raven_${name} PRIVATE
			"--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
		set_property(TARGET raven_${name} APPEND PROPERTY LINK_DEPENDS
			"${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
		# Bundle runtime assets with their original relative directory structure.
		file(GLOB_RECURSE assets CONFIGURE_DEPENDS
			"${name}/*.xml" "${name}/*.svg" "${name}/*.md"
			"${name}/*.frag" "${name}/*.png" "${name}/*.jpg"
			"${name}/*.jpeg" "${name}/*.webp" "${name}/*.gif" "${name}/samples/*")
		foreach(asset IN LISTS assets)
			file(RELATIVE_PATH relative "${CMAKE_CURRENT_SOURCE_DIR}" "${asset}")
			target_link_options(raven_${name} PRIVATE
				"SHELL:--preload-file \"${asset}@/examples/${relative}\"")
			set_property(TARGET raven_${name} APPEND PROPERTY LINK_DEPENDS "${asset}")
		endforeach()
	else()
		target_compile_definitions(raven_${name} PRIVATE
			RAVEN_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/${name}")
	endif()
	if(MSVC)
		target_compile_definitions(raven_${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
	else()
		target_link_libraries(raven_${name} PRIVATE m)
	endif()
endfunction()

raven_add_example(hello hello/main.c)
raven_add_example(editor editor/main.c editor/editor.xml)
raven_add_example(glass glass/main.c glass/glass.xml)
raven_add_example(notepad notepad/main.c notepad/notepad.xml)
raven_add_example(animations animations/main.c animations/animations.xml)
raven_add_example(workspace workspace/main.c workspace/workspace.xml)
raven_add_example(components components/main.c components/components.xml components/widgets.xml)
raven_add_example(memory memory/main.c)
raven_add_example(studio studio/main.c studio/studio.xml studio/clock.svg)
raven_add_example(themes themes/main.c themes/themes.xml themes/ocean.xml themes/paper.xml themes/tweaks.xml themes/notes.md)
raven_add_example(overlay overlay/main.c overlay/overlay.xml)
raven_add_example(effects effects/main.c effects/effects.xml effects/wave.frag effects/liquid.frag)
raven_add_example(plot plot/main.c plot/plot.xml)
raven_add_example(charts charts/main.c charts/charts.xml)
raven_add_example(pdf pdf/main.c pdf/pdf.xml)
raven_add_example(map map/main.c map/world.h map/map.xml)
raven_add_example(carousel carousel/main.c carousel/carousel.xml)
raven_add_example(transfer transfer/main.c transfer/transfer.xml)
raven_add_example(icons icons/main.c icons/icons.xml)
raven_add_example(notifications notifications/main.c notifications/notifications.xml notifications/chat.svg)
raven_add_example(animated_images animated_images/main.c animated_images/animated_images.xml)
raven_add_example(diff_view diff_view/main.c diff_view/diff_view.xml)
raven_add_example(music music/main.c music/library.c music/tags.c music/music.h
	music/ui/main.xml music/ui/widgets.xml music/ui/header.xml music/ui/sidebar.xml music/ui/home.xml
	music/ui/collection.xml music/ui/search.xml music/ui/library.xml music/ui/settings.xml
	music/ui/nowplaying.xml music/ui/player.xml music/ui/island.xml
	music/themes/nocturne.xml music/themes/aurora.xml music/themes/daylight.xml music/themes/styles.xml)

# A page shown by the web engine of the system (a message where the system has none Raven embeds).
if(NOT EMSCRIPTEN)
	raven_add_example(web_view web_view/main.c web_view/web_view.xml web_view/pages/welcome.html)
endif()

# A test of an interface (raven_testing.h), run by ctest too.
if(NOT EMSCRIPTEN)
	raven_add_example(ui_test ui_test/main.c ui_test/login.xml)
	add_test(NAME example_ui_test COMMAND raven_ui_test)
	set_tests_properties(example_ui_test PROPERTIES SKIP_RETURN_CODE 77)
endif()

# The UI of the iOS and Android showcase in a desktop window (examples/mobile/README.md).
if(NOT EMSCRIPTEN)
	raven_add_example(mobile mobile/desktop/main.c mobile/shared/showcase.c mobile/shared/showcase.h)
	target_include_directories(raven_mobile PRIVATE mobile/shared)
endif()

# Any application driven from outside, without a change of its code (RAVEN_HEADLESS, RAVEN_SCRIPT):
# hello, which names none of its widgets, by the script of headless/hello.script. Without a GPU the
# run goes on with the null backend: the checks and the tree, no pictures.
if(NOT EMSCRIPTEN)
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/headless_output")
	add_test(NAME example_headless_hello COMMAND "${CMAKE_COMMAND}" -E env RAVEN_HEADLESS=1
		"RAVEN_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/headless/hello.script" "$<TARGET_FILE:raven_hello>")
	set_tests_properties(example_headless_hello PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/headless_output")
endif()

# Web pages in the interface: needs the optional library (RAVEN_BUILD_WEB).
if(NOT EMSCRIPTEN AND TARGET raven_web)
	raven_add_example(browser browser/main.c browser/browser.xml browser/pages/welcome.html browser/pages/second.html)
	target_link_libraries(raven_browser PRIVATE raven::web)
endif()

# The Vulkan example calls Vulkan itself, so it needs the Vulkan library
# (the Vulkan SDK on Windows, libvulkan-dev on Linux). Raven does not.
if(NOT EMSCRIPTEN AND RAVEN_BACKEND_VULKAN)
	find_package(Vulkan QUIET)
	if(Vulkan_FOUND)
		raven_add_example(viewport_vulkan viewport_vulkan/main.c viewport_vulkan/shaders_spv.h)
		target_link_libraries(raven_viewport_vulkan PRIVATE Vulkan::Vulkan)
		if(NOT WIN32)
			find_package(Threads REQUIRED)
			target_link_libraries(raven_viewport_vulkan PRIVATE Threads::Threads)
		endif()
	else()
		message(STATUS "Raven: example viewport_vulkan skipped (Vulkan library not found)")
	endif()
endif()

# The OpenGL example creates its context on the native child window of a viewport.
set(OpenGL_GL_PREFERENCE LEGACY)
if(NOT EMSCRIPTEN)
	find_package(OpenGL QUIET)
endif()
if(NOT EMSCRIPTEN AND OpenGL_FOUND AND (WIN32 OR RAVEN_PLATFORM_SELECTED STREQUAL "x11"))
	raven_add_example(viewport_opengl viewport_opengl/main.c)
	target_link_libraries(raven_viewport_opengl PRIVATE OpenGL::GL)
	if(WIN32)
		target_link_libraries(raven_viewport_opengl PRIVATE gdi32 user32)
	else()
		target_include_directories(raven_viewport_opengl PRIVATE ${X11_INCLUDE_DIR})
		target_link_libraries(raven_viewport_opengl PRIVATE ${X11_X11_LIB})
	endif()
else()
	message(STATUS "Raven: example viewport_opengl skipped (OpenGL not found)")
endif()

if(EMSCRIPTEN)
	if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
		set(RAVEN_GALLERY_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
	else()
		set(RAVEN_GALLERY_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}")
	endif()
	configure_file(web/index.html "${RAVEN_GALLERY_OUTPUT}/index.html" COPYONLY)
	message(STATUS "Raven: browser gallery includes 14 portable examples; native Chromium embedding, Vulkan and native-child OpenGL examples are unavailable")
endif()
