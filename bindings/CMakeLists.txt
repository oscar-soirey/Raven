# Tests of the bindings: each example of bindings/ run with --test, where the
# tools of its language are installed. They only exist when Raven builds its
# tests; using a binding needs none of this.
#
#   bindings_up_to_date   bindings/ is what tools/bindgen writes from the headers
#   binding_cpp           needs a C++ compiler
#   binding_python        needs Python 3 and the shared library (RAVEN_BUILD_SHARED); not on Windows
#   binding_rust          needs cargo and the shared library; not on Windows
#   binding_go            needs go, a C compiler for cgo, and the shared library; not on Windows
#   binding_java          needs a JDK 22 or later and the shared library; not on Windows
#   binding_odin          needs odin and the shared library; not on Windows
#   binding_bun           needs bun and the shared library; not on Windows
#
# C#, Zig and LuaJIT have no test here.

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
	add_test(NAME bindings_up_to_date
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/bindgen/bindgen.py --check)
endif()

include(CheckLanguage)
check_language(CXX)
if(CMAKE_CXX_COMPILER)
	enable_language(CXX)
	add_executable(raven_binding_cpp cpp/example.cpp cpp/raven.hpp)
	target_include_directories(raven_binding_cpp PRIVATE cpp)
	target_link_libraries(raven_binding_cpp PRIVATE raven::raven)
	set_target_properties(raven_binding_cpp PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON FOLDER "bindings")
	add_test(NAME binding_cpp COMMAND raven_binding_cpp --test)
endif()

# (on Windows these are left to be run by hand: what they need to find the
# DLL and its import library depends on how Python, cargo and go were installed)
if(RAVEN_BUILD_SHARED AND NOT WIN32)
	set(RAVEN_BINDING_ENV
		"RAVEN_LIBRARY=$<TARGET_FILE:raven>"
		"RAVEN_LIB_DIR=$<TARGET_FILE_DIR:raven>"
		"LD_LIBRARY_PATH=$<TARGET_FILE_DIR:raven>"
		"CGO_CFLAGS=-I${CMAKE_SOURCE_DIR}/include"
		"CGO_LDFLAGS=-L$<TARGET_FILE_DIR:raven>")

	if(Python3_FOUND)
		add_test(NAME binding_python
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/python/example.py --test)
	endif()

	find_program(RAVEN_CARGO cargo)
	if(RAVEN_CARGO)
		add_test(NAME binding_rust
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} "CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}/rust"
				${RAVEN_CARGO} run --offline --quiet --example hello -- --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rust)
	endif()

	find_program(RAVEN_GO go)
	if(RAVEN_GO)
		add_test(NAME binding_go
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} "GOFLAGS=-mod=mod" ${RAVEN_GO} run ./example --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/go)
	endif()

	# the class is compiled by a first test, which the example (a source file) then uses
	find_package(Java 22 COMPONENTS Runtime Development QUIET)
	if(Java_FOUND)
		add_test(NAME binding_java_compile
			COMMAND ${Java_JAVAC_EXECUTABLE} -encoding UTF-8 -d ${CMAKE_CURRENT_BINARY_DIR}/java
				${CMAKE_CURRENT_SOURCE_DIR}/java/src/main/java/raven/R.java)
		set_tests_properties(binding_java_compile PROPERTIES FIXTURES_SETUP raven_java)
		add_test(NAME binding_java
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${Java_JAVA_EXECUTABLE} --enable-native-access=ALL-UNNAMED
				-cp ${CMAKE_CURRENT_BINARY_DIR}/java ${CMAKE_CURRENT_SOURCE_DIR}/java/example/Example.java --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		set_tests_properties(binding_java PROPERTIES FIXTURES_REQUIRED raven_java)
	endif()

	find_program(RAVEN_ODIN odin)
	if(RAVEN_ODIN)
		add_test(NAME binding_odin
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_ODIN} run example
				-out:${CMAKE_CURRENT_BINARY_DIR}/raven_binding_odin "-extra-linker-flags:-L$<TARGET_FILE_DIR:raven>" -- --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/odin)
	endif()

	find_program(RAVEN_BUN bun)
	if(RAVEN_BUN)
		add_test(NAME binding_bun
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_BUN} run ${CMAKE_CURRENT_SOURCE_DIR}/bun/src/example.ts --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()
endif()
