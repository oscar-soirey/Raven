# The addon of the Node.js binding (raven_napi.node), from src/raven_napi.c.
#
# It does not link Raven: it opens the shared library at run time. It only needs
# node_api.h, which an installation of Node.js has beside its binary; set
# RAVEN_NODE_INCLUDE_DIR where it does not (a checkout of node-api-headers).
#
#   cmake -S bindings/node -B bindings/node/build -DCMAKE_BUILD_TYPE=Release
#   cmake --build bindings/node/build
#
# (Windows needs the import library of node.exe to link an addon: not done here,
# see the README.)

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

if(NOT RAVEN_NODE_INCLUDE_DIR OR WIN32)
	message(STATUS "Raven: the addon of the Node.js binding is left out (node_api.h not found, or Windows)")
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
else()
	target_link_libraries(raven_node PRIVATE ${CMAKE_DL_LIBS})
endif()
