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
#   browser          web pages in the interface, with the optional library raven_web
#   viewport_vulkan  a Vulkan scene shown in the UI (offscreen image + in-pass draw)
#   viewport_opengl  an OpenGL scene shown in a native child window

function(raven_add_example name)
	add_executable(raven_${name} ${ARGN} common/example.h)
	target_link_libraries(raven_${name} PRIVATE raven::raven)
	set_target_properties(raven_${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "examples")
	# where the example finds its data files when started from any folder
	target_compile_definitions(raven_${name} PRIVATE RAVEN_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/${name}")
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
raven_add_example(studio studio/main.c studio/studio.xml)
raven_add_example(themes themes/main.c themes/themes.xml themes/ocean.xml themes/paper.xml themes/tweaks.xml themes/notes.md)
raven_add_example(overlay overlay/main.c overlay/overlay.xml)
raven_add_example(effects effects/main.c effects/effects.xml effects/wave.frag effects/liquid.frag)

# Web pages in the interface: needs the optional library (RAVEN_BUILD_WEB).
if(TARGET raven_web)
	raven_add_example(browser browser/main.c browser/browser.xml browser/pages/welcome.html browser/pages/second.html)
	target_link_libraries(raven_browser PRIVATE raven::web)
endif()

# The Vulkan example calls Vulkan itself, so it needs the Vulkan library
# (the Vulkan SDK on Windows, libvulkan-dev on Linux). Raven does not.
if(RAVEN_BACKEND_VULKAN)
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
find_package(OpenGL QUIET)
if(OpenGL_FOUND AND (WIN32 OR RAVEN_PLATFORM_SELECTED STREQUAL "x11"))
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
