# Fuzzers (README.md of this folder).
#
#   fuzz_xml      documents, interfaces from XML       fuzz_font    TrueType, CFF, color fonts
#   fuzz_svg      SVG (nanosvg, tamed by Raven)         fuzz_image   PNG, JPEG, GIF, APNG...: still and animated
#   fuzz_png      PNG encoder, round trip               fuzz_diff    unified patches, diff round trip
#   fuzz_script   Raven Script, compiled and run
#
# RAVEN_BUILD_FUZZERS=ON (clang): libFuzzer programs; the whole library is
# instrumented (top CMakeLists.txt). Otherwise, in a build of the tests with
# the static library, the same targets get a main() of their own
# (fuzz_main.c). Either way the test fuzz_<name>_corpus runs each file of
# corpus/<name> once: an input that once crashed, kept there, stays a test.

set(RAVEN_FUZZERS xml font svg image png diff script)

foreach(name ${RAVEN_FUZZERS})
	if(RAVEN_BUILD_FUZZERS)
		add_executable(fuzz_${name} fuzz_${name}.c fuzz.h)
		target_link_options(fuzz_${name} PRIVATE -fsanitize=fuzzer)
	else()
		add_executable(fuzz_${name} fuzz_${name}.c fuzz.h fuzz_main.c)
	endif()
	target_link_libraries(fuzz_${name} PRIVATE raven::raven)
	if(NOT WIN32)
		target_link_libraries(fuzz_${name} PRIVATE m)
	endif()
	if(MSVC)
		target_compile_definitions(fuzz_${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
	endif()
	if(NOT RAVEN_WITH_STB_IMAGE)
		target_compile_definitions(fuzz_${name} PRIVATE RAVEN_TEST_NO_IMAGE_DECODER)
	endif()
	set_target_properties(fuzz_${name} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON FOLDER "tests/fuzz")
	file(GLOB seeds LIST_DIRECTORIES false CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/corpus/${name}/*")
	list(SORT seeds)
	if(name STREQUAL "font")
		# the font of the library is a seed too
		list(APPEND seeds "${PROJECT_SOURCE_DIR}/third_party/roboto/Roboto-Regular-subset.ttf")
	endif()
	if(seeds)
		add_test(NAME fuzz_${name}_corpus COMMAND fuzz_${name} ${seeds})
	endif()
endforeach()

