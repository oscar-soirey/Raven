# Tests.
#
#   test_layout, test_widgets, test_xml, test_threads, test_memory, test_animation,
#   test_text_editor, test_shell, test_dialogs, test_themes, test_markdown, test_tools,
#   test_locale, test_datetime, test_fonts, test_touch, test_more_widgets, test_notifications,
#   test_pdf, test_pdf_document, test_chart, test_carousel, test_animated_images, test_diff_view,
#   test_diff_engine, test_system_web_view, test_deep_trees, test_lifecycle, test_package,
#   test_testing, test_table_edit, test_combo_search, test_transfer, test_device, test_preedit,
#   test_icons, test_spell, test_headless, test_web_input
#       run headless (null platform + null backend): no display and no GPU are
#       needed, so they run on any build machine.
#   headless_cli_tree, headless_cli_script, headless_cli_fail, headless_cli_syntax,
#   headless_cli_quit, headless_cli_snap
#       headless_app.c, an ordinary application, driven from outside by the
#       environment (headless_cli.cmake); the pictures of headless_cli_snap need
#       a GPU backend (skipped without).
#   test_wayland
#       the Wayland platform against a small compositor run in the test itself
#       (libwayland-server): skipped where it is not installed.
#   test_render, test_render_metal
#       render real frames with the Vulkan backend, and with the Metal backend
#       (macOS, iOS), and check pixels. They are reported as skipped when the
#       machine has no usable driver.
#   test_golden
#       renders a form with the GPU backend of the platform and compares it with
#       the golden images of tests/goldens/ (one per OS and backend); skipped
#       without a GPU or without goldens for this OS and backend.
#
#   raven_benchmark
#       not a test: prints layout and paint timings for a large widget tree.
#
#   ctest --output-on-failure

function(raven_add_test name)
	add_executable(${name} ${name}.c test.h)
	target_link_libraries(${name} PRIVATE raven::raven)
	set_target_properties(${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests")
	if(NOT WIN32)
		target_link_libraries(${name} PRIVATE m)
	endif()
	if(MSVC)
		target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
	endif()
	if(NOT RAVEN_WITH_STB_IMAGE)
		target_compile_definitions(${name} PRIVATE RAVEN_TEST_NO_IMAGE_DECODER)
	endif()
	add_test(NAME ${name} COMMAND ${name})
endfunction()

raven_add_test(test_layout)
raven_add_test(test_widgets)
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_menu_decorations)
endif()
raven_add_test(test_xml)
raven_add_test(test_threads)
raven_add_test(test_memory)
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_handles)
endif()
raven_add_test(test_animation)
raven_add_test(test_text_editor)
raven_add_test(test_shell)
raven_add_test(test_dock_tearing)
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_dock_in_window)
endif()
raven_add_test(test_editor_features)
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_drop_highlight)
endif()
raven_add_test(test_dialogs)
raven_add_test(test_themes)
raven_add_test(test_markdown)
raven_add_test(test_tools)
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_draw_shadow)
	raven_add_test(test_effect_hover)
	target_compile_definitions(test_effect_hover PRIVATE "RAVEN_TEST_LIQUID=\"${PROJECT_SOURCE_DIR}/examples/effects/liquid.spv\"")
endif()
raven_add_test(test_locale)
# the clock of the locale, the TimePicker on 12 hours, time zones read from tests/data/zoneinfo
raven_add_test(test_datetime)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_datetime_output")
target_compile_definitions(test_datetime PRIVATE
	"RAVEN_TEST_ZONEINFO=\"${CMAKE_CURRENT_SOURCE_DIR}/data/zoneinfo\""
	"RAVEN_TEST_OUTPUT=\"${CMAKE_CURRENT_BINARY_DIR}/test_datetime_output\"")
if(RAVEN_BUILD_SHARED)
	target_compile_definitions(test_datetime PRIVATE RAVEN_TEST_SHARED)
endif()
if(NOT RAVEN_BUILD_SHARED)
	# Tests the shared screen-reader tree/actions used by all native platforms.
	raven_add_test(test_accessibility)
endif()
raven_add_test(test_host_platform)
# permissions, pickers, camera, share, haptics, downloads: a phone played by the host platform, then a desktop
raven_add_test(test_device)
if(NOT RAVEN_BUILD_SHARED)
	# the bookkeeping of the input methods of Linux (text-input-v3, XIM), apart from the window systems
	raven_add_test(test_preedit)
endif()
if(RAVEN_PLATFORM_SELECTED STREQUAL "cocoa")
	# Real AppKit key events through RavenCocoaView, with the null renderer (no Vulkan).
	add_executable(test_cocoa_input test_cocoa_input.m test.h)
	target_link_libraries(test_cocoa_input PRIVATE raven::raven ${RAVEN_APPKIT_FRAMEWORK})
	add_test(NAME test_cocoa_input COMMAND test_cocoa_input)
	# The real NSPasteboard (the general one is given back at the end) and the start of a drag session out of a window.
	add_executable(test_cocoa_transfer test_cocoa_transfer.m test.h)
	target_link_libraries(test_cocoa_transfer PRIVATE raven::raven ${RAVEN_APPKIT_FRAMEWORK})
	add_test(NAME test_cocoa_transfer COMMAND test_cocoa_transfer)
	# a real WKWebView in a Cocoa window: pages given as text, scripts, messages, where the view is placed
	add_executable(test_cocoa_web_view test_cocoa_web_view.m test.h)
	target_link_libraries(test_cocoa_web_view PRIVATE raven::raven ${RAVEN_APPKIT_FRAMEWORK})
	add_test(NAME test_cocoa_web_view COMMAND test_cocoa_web_view)
	set_tests_properties(test_cocoa_web_view PROPERTIES TIMEOUT 180)
endif()
if(RAVEN_PLATFORM_SELECTED STREQUAL "win32")
	# Real GDI redirection surface behind DWM backdrops; no Vulkan driver required.
	raven_add_test(test_win32_backdrop)
	target_link_libraries(test_win32_backdrop PRIVATE user32 gdi32)
	set_tests_properties(test_win32_backdrop PROPERTIES SKIP_RETURN_CODE 77)
endif()
raven_add_test(test_touch)
# the text field of the browser platform: UTF-16 offsets, the text around the caret, field attributes
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_web_input)
endif()
if(NOT RAVEN_BUILD_SHARED)
	# pause / resume, low memory, the native surface lost and made again, Back (sent as a phone's window system would)
	raven_add_test(test_lifecycle)
endif()
raven_add_test(test_more_widgets)
# notifications with tags, buttons and an icon, through the fallback toast (the null platform has none of its own)
raven_add_test(test_notifications)
if(RAVEN_BUILD_SHARED)
	# (what the platforms report and share is internal: checked with the static library)
	target_compile_definitions(test_notifications PRIVATE RAVEN_TEST_SHARED)
endif()
raven_add_test(test_chart)
raven_add_test(test_carousel)
# the clipboard formats and the drags out of the windows (the null platform's clipboard)
raven_add_test(test_transfer)
# SystemWebView where the system has no web view (headless): the widget, its API, its XML, its message
raven_add_test(test_system_web_view)
raven_add_test(test_animated_images)
target_sources(test_animated_images PRIVATE anim_foreign_gif.h anim_foreign_expected.h)
raven_add_test(test_diff_view)
if(NOT RAVEN_BUILD_SHARED)
	# the comparison itself (core/diff.c), through its internal header
	raven_add_test(test_diff_engine)
endif()
raven_add_test(test_pdf)
# without PDFium (RAVEN_PDFIUM, or the library next to the test) it is reported as skipped
set_tests_properties(test_pdf PROPERTIES SKIP_RETURN_CODE 77)
raven_add_test(test_map_view)
if(RAVEN_BUILD_SHARED)
	# (what a paint drew and the tree of the screen readers are internal: checked with the static library)
	target_compile_definitions(test_map_view PRIVATE RAVEN_TEST_SHARED)
endif()
# PDF documents written by Raven (read back by PdfView when PDFium is there) and printing to a file
raven_add_test(test_pdf_document)
target_compile_definitions(test_pdf_document PRIVATE "RAVEN_TEST_FONT=\"${PROJECT_SOURCE_DIR}/third_party/roboto/Roboto-Regular-subset.ttf\"")
if(NOT RAVEN_BUILD_SHARED)
	raven_add_test(test_table_features)
	# cells edited in place, frozen columns; editable and searchable combo boxes (the screen reader tree is internal)
	raven_add_test(test_table_edit)
	raven_add_test(test_combo_search)
endif()
# the icons shipped with Raven and SVGs in currentColor, tinted by the widgets (a backend of the test records what is drawn)
raven_add_test(test_icons)
if(NOT RAVEN_WITH_SVG)
	target_compile_definitions(test_more_widgets PRIVATE RAVEN_TEST_NO_SVG)
	target_compile_definitions(test_icons PRIVATE RAVEN_TEST_NO_SVG)
endif()
if(NOT RAVEN_BUILD_SHARED)
	# spell checking with a checker of the test (same results everywhere): underlines, menu, cost of a long text
	raven_add_test(test_spell)
endif()
# the spell checker of the system (macOS, Windows, Enchant / Hunspell): skipped where there is none or no English
raven_add_test(test_spell_native)
set_tests_properties(test_spell_native PROPERTIES SKIP_RETURN_CODE 77)
# raven_testing.h: widgets driven by name, frames until idle, golden images (headless, null backend)
raven_add_test(test_testing)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_testing_output")
target_compile_definitions(test_testing PRIVATE "RAVEN_TEST_OUTPUT=\"${CMAKE_CURRENT_BINARY_DIR}/test_testing_output\"")
# the tree as JSON and the scripts of steps, in the process (headless, null backend)
raven_add_test(test_headless)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_headless_output")
target_compile_definitions(test_headless PRIVATE "RAVEN_TEST_OUTPUT=\"${CMAKE_CURRENT_BINARY_DIR}/test_headless_output\"")
# an ordinary application driven from outside by the environment (RAVEN_HEADLESS, RAVEN_SCRIPT, RAVEN_TREE,
# RAVEN_SNAP): the tree, a script, the exit codes; the pictures need a GPU backend (skipped without)
add_executable(headless_app headless_app.c)
target_link_libraries(headless_app PRIVATE raven::raven)
set_target_properties(headless_app PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests")
foreach(case tree script fail syntax quit snap)
	add_test(NAME headless_cli_${case} COMMAND "${CMAKE_COMMAND}" "-DAPP=$<TARGET_FILE:headless_app>" -DCASE=${case}
		"-DOUT=${CMAKE_CURRENT_BINARY_DIR}/headless_cli_output/${case}" -P "${CMAKE_CURRENT_SOURCE_DIR}/headless_cli.cmake")
endforeach()
set_tests_properties(headless_cli_snap PROPERTIES SKIP_REGULAR_EXPRESSION "SKIPPED")
# trees far deeper than the stack allows a recursion to follow: the stack is made small (by the program itself elsewhere)
raven_add_test(test_deep_trees)
if(MSVC)
	target_link_options(test_deep_trees PRIVATE /STACK:524288)
elseif(WIN32)
	target_link_options(test_deep_trees PRIVATE -Wl,--stack,524288)
endif()
# damaged fonts: cut short and changed at random, loaded and drawn (meant to run under AddressSanitizer too)
raven_add_test(test_fonts)
target_compile_definitions(test_fonts PRIVATE "RAVEN_TEST_FONT=\"${PROJECT_SOURCE_DIR}/third_party/roboto/Roboto-Regular-subset.ttf\"")
if(NOT RAVEN_BUILD_SHARED)
	# the application's package (APK assets, iOS bundle) through a provider of the test (core/file.c)
	raven_add_test(test_package)
	target_compile_definitions(test_package PRIVATE "RAVEN_TEST_FONT=\"${PROJECT_SOURCE_DIR}/third_party/roboto/Roboto-Regular-subset.ttf\"")
endif()

# the Wayland platform against a compositor of its own (skipped without libwayland-server)
if(RAVEN_PLATFORM_SELECTED STREQUAL "x11" AND RAVEN_WAYLAND)
	raven_add_test(test_wayland)
	target_include_directories(test_wayland PRIVATE ${PROJECT_SOURCE_DIR}/src/platform/wayland)
	find_package(Threads REQUIRED)
	target_link_libraries(test_wayland PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
	set_tests_properties(test_wayland PROPERTIES SKIP_RETURN_CODE 77)
	find_package(Python3 COMPONENTS Interpreter QUIET)
	if(Python3_FOUND)
		# wayland_protocols.h is what tools/wayland/gen_wayland.py writes from third_party/wayland-protocols
		add_test(NAME wayland_protocols_up_to_date
			COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/wayland/gen_wayland.py --check)
	endif()
endif()

# one executable for both GPU backends: RAVEN_BACKEND picks the one it renders with
if(RAVEN_BACKEND_VULKAN OR RAVEN_BACKEND_METAL)
	raven_add_test(test_render)
	if(RAVEN_BACKEND_VULKAN)
		target_compile_definitions(test_render PRIVATE RAVEN_TEST_VULKAN=1)
		set_tests_properties(test_render PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "RAVEN_BACKEND=vulkan")
	else()
		# (no Vulkan in the library: only the Metal test is run)
		target_compile_definitions(test_render PRIVATE RAVEN_TEST_VULKAN=0)
		set_tests_properties(test_render PROPERTIES DISABLED TRUE)
	endif()
	if(RAVEN_BACKEND_METAL)
		add_test(NAME test_render_metal COMMAND test_render)
		set_tests_properties(test_render_metal PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "RAVEN_BACKEND=metal")
		find_package(Python3 COMPONENTS Interpreter QUIET)
		if(Python3_FOUND)
			# generated/shaders.h is what src/render/metal/generate_shaders.py writes from the .metal files
			add_test(NAME metal_shaders_up_to_date
				COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/src/render/metal/generate_shaders.py --check)
		endif()
	endif()
endif()

# golden images rendered by the GPU backend, compared with tests/goldens/ (skipped without a GPU, or without goldens of this OS and backend)
if(RAVEN_BACKEND_VULKAN OR RAVEN_BACKEND_METAL)
	raven_add_test(test_golden)
	target_compile_definitions(test_golden PRIVATE
		"RAVEN_TEST_GOLDENS=\"${CMAKE_CURRENT_SOURCE_DIR}/goldens\""
		"RAVEN_TEST_OUTPUT=\"${CMAKE_CURRENT_BINARY_DIR}/golden_failures\"")
	set_tests_properties(test_golden PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Not a test: prints timings for a large widget tree (see the top of benchmark.c).
add_executable(raven_benchmark benchmark.c)
target_link_libraries(raven_benchmark PRIVATE raven::raven)
set_target_properties(raven_benchmark PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests")
if(MSVC)
	target_compile_definitions(raven_benchmark PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
