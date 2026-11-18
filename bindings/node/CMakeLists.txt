# The addon of the Node.js binding (raven_napi.node), from src/raven_napi.c.
#
# It does not link Raven: it opens the shared library at run time. It only needs
# node_api.h, which an installation of Node.js has beside its binary; set
# RAVEN_NODE_INCLUDE_DIR where it does not (a checkout of node-api-headers).
#
#   cmake -S bindings/node -B bindings/node/build -DCMAKE_BUILD_TYPE=Release
#   cmake --build bindings/node/build
#
# On Windows an addon also links the import library of node.exe (node.lib), and
# an installation of Node.js has neither that nor the headers. By default both
# are downloaded from nodejs.org, for the version of the node found here, into
# the build folder, and checked against the SHASUMS256.txt of that release. To
# build without a network, give RAVEN_NODE_INCLUDE_DIR and RAVEN_NODE_LIBRARY,
# and turn RAVEN_NODE_DOWNLOAD off. See the README.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	cmake_minimum_required(VERSION 3.16)
	project(raven_node C)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

find_program(RAVEN_NODE node)
if(RAVEN_NODE AND NOT RAVEN_NODE_INCLUDE_DIR)
	get_filename_component(RAVEN_NODE_BIN "${RAVEN_NODE}" REALPATH)
	get_filename_component(RAVEN_NODE_PREFIX "${RAVEN_NODE_BIN}" DIRECTORY)
	get_filename_component(RAVEN_NODE_PREFIX "${RAVEN_NODE_PREFIX}" DIRECTORY)
	find_path(RAVEN_NODE_INCLUDE_DIR node_api.h HINTS "${RAVEN_NODE_PREFIX}/include/node" PATH_SUFFIXES node)
endif()

# Windows: the headers and node.lib of the node found here, from nodejs.org.
# Each file is checked against the SHASUMS256.txt of its release, and is only
# downloaded again when the version of node changes. Sets RAVEN_NODE_INCLUDE_DIR
# and RAVEN_NODE_LIBRARY, those not given, or warns and leaves them unset.
function(raven_node_download)
	execute_process(COMMAND "${RAVEN_NODE}" -p process.version
		OUTPUT_VARIABLE version OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE result)
	if(NOT result EQUAL 0 OR NOT version MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
		message(WARNING "Raven: could not read the version of ${RAVEN_NODE}")
		return()
	endif()

	# node.lib of the processor the addon is built for (the compiler's, not the host's)
	set(arch "${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
	if(NOT arch)
		set(arch "${CMAKE_SYSTEM_PROCESSOR}")
	endif()
	string(TOLOWER "${arch}" arch)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(arch x86)
	elseif(arch MATCHES "arm64|aarch64")
		set(arch arm64)
	else()
		set(arch x64)
	endif()

	set(url "https://nodejs.org/dist/${version}")
	set(dir "${CMAKE_CURRENT_BINARY_DIR}/node-${version}")
	set(headers "node-${version}-headers.tar.gz")
	set(library "win-${arch}/node.lib")

	if(NOT EXISTS "${dir}/SHASUMS256.txt")
		file(DOWNLOAD "${url}/SHASUMS256.txt" "${dir}/SHASUMS256.txt.part" TLS_VERIFY ON STATUS status)
		list(GET status 0 code)
		if(NOT code EQUAL 0)
			file(REMOVE "${dir}/SHASUMS256.txt.part")
			message(WARNING "Raven: could not download ${url}/SHASUMS256.txt: ${status}")
			return()
		endif()
		file(RENAME "${dir}/SHASUMS256.txt.part" "${dir}/SHASUMS256.txt")
	endif()
	file(STRINGS "${dir}/SHASUMS256.txt" sums)

	foreach(name "${headers}" "${library}")
		set(expected "")
		foreach(line IN LISTS sums)
			if(line MATCHES "^([0-9a-f]+)  (.+)$")
				if(CMAKE_MATCH_2 STREQUAL name)
					set(expected "${CMAKE_MATCH_1}")
				endif()
			endif()
		endforeach()
		if(NOT expected)
			message(WARNING "Raven: ${name} is not in ${url}/SHASUMS256.txt")
			return()
		endif()
		set(actual "")
		if(EXISTS "${dir}/${name}")
			file(SHA256 "${dir}/${name}" actual)
		endif()
		if(NOT actual STREQUAL expected)
			message(STATUS "Raven: downloading ${url}/${name}")
			file(DOWNLOAD "${url}/${name}" "${dir}/${name}" TLS_VERIFY ON STATUS status)
			list(GET status 0 code)
			set(actual "")
			if(code EQUAL 0)
				file(SHA256 "${dir}/${name}" actual)
			endif()
			if(NOT actual STREQUAL expected)
				file(REMOVE "${dir}/${name}")
				message(WARNING "Raven: could not download ${url}/${name} (${status}, SHA-256 ${actual}, expected ${expected})")
				return()
			endif()
			file(REMOVE_RECURSE "${dir}/node-${version}")
		endif()
	endforeach()

	if(NOT EXISTS "${dir}/node-${version}/include/node/node_api.h")
		execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${headers}" WORKING_DIRECTORY "${dir}" RESULT_VARIABLE result)
		if(NOT result EQUAL 0 OR NOT EXISTS "${dir}/node-${version}/include/node/node_api.h")
			message(WARNING "Raven: could not extract ${dir}/${headers}")
			return()
		endif()
	endif()

	if(NOT RAVEN_NODE_INCLUDE_DIR)
		set(RAVEN_NODE_INCLUDE_DIR "${dir}/node-${version}/include/node" PARENT_SCOPE)
	endif()
	if(NOT RAVEN_NODE_LIBRARY)
		set(RAVEN_NODE_LIBRARY "${dir}/${library}" PARENT_SCOPE)
	endif()
endfunction()

if(WIN32)
	set(RAVEN_NODE_LIBRARY "" CACHE FILEPATH "Windows: the import library of node.exe (node.lib) that the Node.js addon links")
	option(RAVEN_NODE_DOWNLOAD "Windows: download the headers and node.lib of the Node.js found here, when they are not given" ON)
	if(RAVEN_NODE AND RAVEN_NODE_DOWNLOAD AND NOT (RAVEN_NODE_INCLUDE_DIR AND RAVEN_NODE_LIBRARY))
		raven_node_download()
	endif()
	if(NOT RAVEN_NODE_INCLUDE_DIR OR NOT RAVEN_NODE_LIBRARY)
		message(STATUS "Raven: the addon of the Node.js binding is left out (node_api.h or node.lib not found)")
		return()
	endif()
elseif(NOT RAVEN_NODE_INCLUDE_DIR)
	message(STATUS "Raven: the addon of the Node.js binding is left out (node_api.h not found)")
	return()
endif()

add_library(raven_node MODULE src/raven_napi.c)
set_target_properties(raven_node PROPERTIES PREFIX "" SUFFIX ".node" OUTPUT_NAME raven_napi
	C_STANDARD 11 C_EXTENSIONS ON C_VISIBILITY_PRESET hidden FOLDER "bindings")
target_include_directories(raven_node PRIVATE ${RAVEN_NODE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_compile_definitions(raven_node PRIVATE NAPI_DISABLE_CPP_EXCEPTIONS)
if(APPLE)
	# the functions of Node-API are in the process that loads the addon
	target_link_options(raven_node PRIVATE -undefined dynamic_lookup)
elseif(WIN32)
	# node.lib imports them from node.exe by that name. Visual C++ loads them
	# late (/DELAYLOAD), and win_delay_load_hook.c gives the program that runs
	# instead, so the addon also loads in an executable of another name, as
	# node-gyp does. MinGW has no delayed loading: there it needs node.exe.
	target_link_libraries(raven_node PRIVATE "${RAVEN_NODE_LIBRARY}")
	if(MSVC)
		target_sources(raven_node PRIVATE win_delay_load_hook.c)
		target_link_libraries(raven_node PRIVATE delayimp)
		target_link_options(raven_node PRIVATE /DELAYLOAD:node.exe)
	endif()
else()
	target_link_libraries(raven_node PRIVATE ${CMAKE_DL_LIBS})
endif()
