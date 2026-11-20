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
#   binding_odin_wrapper, binding_odin_wrapper_test    the same needs: the example and the tests of odin/raven_odin
#   binding_bun           needs bun and the shared library; not on Windows
#   binding_node          needs node and node_api.h (the addon is built here) and the shared library;
#                         on Windows also node.lib, both downloaded by default (see node/CMakeLists.txt)
#   binding_node_bun      the same addon and test, run by bun instead; not on Windows
#   binding_d             needs dub and a D compiler, and the shared library; not on Windows
#   binding_nim           needs nim and the shared library; not on Windows
#   binding_swift         needs swift (5.9 or later) and the shared library; not on Windows
#   binding_haskell       needs ghc and cabal, and the shared library; not on Windows
#   binding_zig           needs zig: compiles and checks every declaration of zig/raven.zig (no binary is made: nothing is linked or run)
#   binding_lua_embedded  needs the headers and the library of Lua (5.1 to 5.5): a program that embeds Lua
#                         and links Raven (lua/host.c, with lua/raven_lua.c compiled in) runs lua/example.lua
#   binding_lua           the same, and the interpreter of that version of Lua and the shared library: the
#                         module raven.so is built and loaded by require("raven"); not on Windows
#
# C# and LuaJIT have no test here. Lua is found by find_package(Lua): set
# LUA_INCLUDE_DIR and LUA_LIBRARY to choose another version than the newest.

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
	add_test(NAME bindings_up_to_date
		COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/bindgen/bindgen.py --check)
endif()

find_program(RAVEN_ZIG zig)
if(RAVEN_ZIG)
	add_test(NAME binding_zig
		COMMAND ${RAVEN_ZIG} test ${CMAKE_CURRENT_SOURCE_DIR}/zig/check.zig --test-no-exec -fno-emit-bin
			--cache-dir ${CMAKE_CURRENT_BINARY_DIR}/zig-cache
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Lua: raven_lua.c compiled into a program that embeds Lua (RAVEN_LUA_LINK: it calls
# the Raven it is linked with), as a game engine or a tool with Lua inside would
find_package(Lua QUIET)
if(LUA_FOUND)
	add_executable(raven_binding_lua_host lua/host.c lua/raven_lua.c lua/raven_lua.h)
	target_include_directories(raven_binding_lua_host PRIVATE lua ${LUA_INCLUDE_DIR})
	target_compile_definitions(raven_binding_lua_host PRIVATE RAVEN_LUA_LINK)
	target_link_libraries(raven_binding_lua_host PRIVATE raven::raven ${LUA_LIBRARIES})
	set_target_properties(raven_binding_lua_host PROPERTIES C_STANDARD 11 FOLDER "bindings")
	add_test(NAME binding_lua_embedded
		COMMAND raven_binding_lua_host ${CMAKE_CURRENT_SOURCE_DIR}/lua/example.lua --test
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
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
		"DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:raven>"
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
				-out:${CMAKE_CURRENT_BINARY_DIR}/raven_binding_odin "-extra-linker-flags:-L$<TARGET_FILE_DIR:raven> -Wl,-rpath,$<TARGET_FILE_DIR:raven>" -- --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/odin)
		# raven_odin, the layer over the generated package: its example, and its tests
		# (one at a time: Raven has one UI thread)
		add_test(NAME binding_odin_wrapper
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_ODIN} run raven_odin/example
				-out:${CMAKE_CURRENT_BINARY_DIR}/raven_binding_odin_wrapper "-extra-linker-flags:-L$<TARGET_FILE_DIR:raven> -Wl,-rpath,$<TARGET_FILE_DIR:raven>" -- --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/odin)
		add_test(NAME binding_odin_wrapper_test
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_ODIN} test raven_odin
				-out:${CMAKE_CURRENT_BINARY_DIR}/raven_binding_odin_wrapper_test -define:ODIN_TEST_THREADS=1
				"-extra-linker-flags:-L$<TARGET_FILE_DIR:raven> -Wl,-rpath,$<TARGET_FILE_DIR:raven>"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/odin)
	endif()

	find_program(RAVEN_BUN bun)
	if(RAVEN_BUN)
		add_test(NAME binding_bun
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_BUN} run ${CMAKE_CURRENT_SOURCE_DIR}/bun/src/example.ts --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# (--temp-build: nothing is built inside the source tree; the example reads RAVEN_LIB_DIR)
	find_program(RAVEN_DUB dub)
	if(RAVEN_DUB)
		add_test(NAME binding_d
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_DUB} run --root=${CMAKE_CURRENT_SOURCE_DIR}/d/example
				--temp-build -- --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# (the nimcache and the program go to the build tree; the example reads RAVEN_LIB_DIR)
	find_program(RAVEN_NIM nim)
	if(RAVEN_NIM)
		add_test(NAME binding_nim
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_NIM} c -r --hints:off
				--nimcache:${CMAKE_CURRENT_BINARY_DIR}/nim-cache --out:${CMAKE_CURRENT_BINARY_DIR}/nim-hello
				${CMAKE_CURRENT_SOURCE_DIR}/nim/example/hello.nim --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# (the package reads RAVEN_LIB_DIR to link the library and find it at run time)
	find_program(RAVEN_SWIFT swift)
	if(RAVEN_SWIFT)
		add_test(NAME binding_swift
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_SWIFT} run
				--package-path ${CMAKE_CURRENT_SOURCE_DIR}/swift --scratch-path ${CMAKE_CURRENT_BINARY_DIR}/swift hello --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# (cabal resolves only base and containers, which come with GHC: no download)
	find_program(RAVEN_GHC ghc)
	find_program(RAVEN_CABAL cabal)
	if(RAVEN_GHC AND RAVEN_CABAL)
		add_test(NAME binding_haskell
			COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV} ${RAVEN_CABAL} run raven-hello
				--builddir=${CMAKE_CURRENT_BINARY_DIR}/haskell --extra-lib-dirs=$<TARGET_FILE_DIR:raven>
				"--ghc-options=-optl-Wl,-rpath,$<TARGET_FILE_DIR:raven>" -- --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/haskell)
	endif()

	# Lua: the module raven.so, as LuaRocks builds it (it opens the shared library:
	# RAVEN_LIBRARY), loaded by the interpreter of the version of the headers
	if(LUA_FOUND)
		find_package(Threads REQUIRED)
		add_library(raven_lua MODULE lua/raven_lua.c lua/raven_lua.h)
		set_target_properties(raven_lua PROPERTIES PREFIX "" OUTPUT_NAME raven SUFFIX ".so"
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lua
			C_STANDARD 11 C_VISIBILITY_PRESET hidden FOLDER "bindings")
		target_include_directories(raven_lua PRIVATE lua ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include)
		target_link_libraries(raven_lua PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
		if(APPLE)
			# the functions of Lua are in the program that loads the module
			target_link_options(raven_lua PRIVATE -undefined dynamic_lookup)
		endif()
		set(RAVEN_LUA_VERSION "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
		find_program(RAVEN_LUA NAMES lua${RAVEN_LUA_VERSION} lua-${RAVEN_LUA_VERSION} lua${LUA_VERSION_MAJOR}${LUA_VERSION_MINOR} lua)
		if(RAVEN_LUA)
			# (lua -v writes to stderr before Lua 5.2)
			execute_process(COMMAND ${RAVEN_LUA} -v OUTPUT_VARIABLE RAVEN_LUA_V ERROR_VARIABLE RAVEN_LUA_V_ERR)
			if("${RAVEN_LUA_V}${RAVEN_LUA_V_ERR}" MATCHES "Lua ${LUA_VERSION_MAJOR}\\.${LUA_VERSION_MINOR}")
				add_test(NAME binding_lua
					COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_BINDING_ENV}
						"LUA_CPATH=${CMAKE_CURRENT_BINARY_DIR}/lua/?.so"
						"LUA_CPATH_${LUA_VERSION_MAJOR}_${LUA_VERSION_MINOR}=${CMAKE_CURRENT_BINARY_DIR}/lua/?.so"
						${RAVEN_LUA} ${CMAKE_CURRENT_SOURCE_DIR}/lua/example.lua --test
					WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
			else()
				message(STATUS "Raven: binding_lua is left out (${RAVEN_LUA} is not Lua ${RAVEN_LUA_VERSION}, the version of the headers)")
			endif()
		endif()
	endif()

	add_subdirectory(node)
	if(TARGET raven_node)
		set(RAVEN_NODE_ENV "RAVEN_ADDON=$<TARGET_FILE:raven_node>" ${RAVEN_BINDING_ENV})
		if(RAVEN_NODE)
			add_test(NAME binding_node
				COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_NODE_ENV} ${RAVEN_NODE} --expose-gc ${CMAKE_CURRENT_SOURCE_DIR}/node/src/example.mjs --test
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		endif()
		if(RAVEN_BUN)
			add_test(NAME binding_node_bun
				COMMAND ${CMAKE_COMMAND} -E env ${RAVEN_NODE_ENV} ${RAVEN_BUN} run ${CMAKE_CURRENT_SOURCE_DIR}/node/src/example.mjs --test
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		endif()
	endif()
endif()

# Windows: only the Node.js binding, whose addon links node.lib there. RAVEN_LIBRARY
# is the full path of the DLL, which is all the addon needs to open it.
if(RAVEN_BUILD_SHARED AND WIN32)
	add_subdirectory(node)
	if(TARGET raven_node AND RAVEN_NODE)
		add_test(NAME binding_node
			COMMAND ${CMAKE_COMMAND} -E env "RAVEN_ADDON=$<TARGET_FILE:raven_node>" "RAVEN_LIBRARY=$<TARGET_FILE:raven>"
				${RAVEN_NODE} --expose-gc ${CMAKE_CURRENT_SOURCE_DIR}/node/src/example.mjs --test
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	endif()
endif()
