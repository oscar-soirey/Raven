# raven.pc: the library for pkg-config (pkgconf), next to the CMake package.
#
#   cc app.c $(pkg-config --cflags --libs raven)
#   cc app.c $(pkg-config --cflags --libs --static raven)     # with libraven.a
#
# What the library links with (RAVEN_LIBRARIES: the frameworks of macOS, -lm,
# -ldl, the threads, X11, the libraries of Windows) is written as flags:
#   - a shared library: in Libs.private, which only --static reads;
#   - a static library (no shared one beside it): in Libs as well, so that
#     --libs alone gives a link line that works with libraven.a.
# X11 is required as the module x11 when the system has x11.pc (Requires.private
# for a shared library, Requires for a static one), -lX11 otherwise.
#
# The prefix is found from the folder of the file (${pcfiledir}), so an
# installed tree can be moved (the packages of the releases, vcpkg) and
# `cmake --install --prefix <folder>` needs nothing more.
#
#   raven_write_pkgconfig(<target> <libraries...>)

function(raven_pkgconfig_flags out_flags out_requires)
	set(flags "")
	set(requires "")
	foreach(library IN LISTS ARGN)
		if(library STREQUAL "")
			continue()
		elseif(library STREQUAL "Threads::Threads")
			if(CMAKE_THREAD_LIBS_INIT)
				list(APPEND flags ${CMAKE_THREAD_LIBS_INIT})
			endif()
		elseif(library MATCHES "^-")
			list(APPEND flags "${library}")
		elseif(library MATCHES "/([^/]+)\\.framework/?$")
			list(APPEND flags "-framework ${CMAKE_MATCH_1}")
		elseif(IS_ABSOLUTE "${library}")
			get_filename_component(name "${library}" NAME_WE)
			string(REGEX REPLACE "^lib" "" name "${name}")
			if(name STREQUAL "X11" AND RAVEN_PKGCONFIG_X11)
				list(APPEND requires "x11")
			else()
				get_filename_component(directory "${library}" DIRECTORY)
				list(FIND CMAKE_C_IMPLICIT_LINK_DIRECTORIES "${directory}" implicit)
				if(implicit EQUAL -1)
					list(APPEND flags "-L${directory}")
				endif()
				list(APPEND flags "-l${name}")
			endif()
		elseif(library MATCHES "^[A-Za-z0-9_.+-]+$" AND NOT library MATCHES "::")
			list(APPEND flags "-l${library}")
		else()
			message(WARNING "Raven: raven.pc: '${library}' is not written in Libs.private (not a library name, a path or a framework)")
		endif()
	endforeach()
	list(REMOVE_DUPLICATES flags)
	list(REMOVE_DUPLICATES requires)
	string(REPLACE ";" " " flags "${flags}")
	string(REPLACE ";" ", " requires "${requires}")
	set(${out_flags} "${flags}" PARENT_SCOPE)
	set(${out_requires} "${requires}" PARENT_SCOPE)
endfunction()

function(raven_write_pkgconfig target)
	include(GNUInstallDirs)
	set(RAVEN_PKGCONFIG_X11 OFF)
	if(ARGN MATCHES "libX11")
		find_package(PkgConfig QUIET)
		if(PKG_CONFIG_FOUND)
			pkg_check_modules(RAVEN_PC_X11 QUIET x11)
			if(RAVEN_PC_X11_FOUND)
				set(RAVEN_PKGCONFIG_X11 ON)
			endif()
		endif()
	endif()
	raven_pkgconfig_flags(private requires ${ARGN})

	# options the programs need too (Emscripten)
	get_target_property(link_options ${target} INTERFACE_LINK_OPTIONS)
	set(public_options "")
	if(link_options)
		string(REPLACE ";" " " public_options "${link_options}")
	endif()

	# from lib/pkgconfig (or lib/<arch>/pkgconfig) up to the prefix
	if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
		set(pc_prefix "${CMAKE_INSTALL_PREFIX}")
		set(pc_libdir "${CMAKE_INSTALL_LIBDIR}")
	else()
		set(pc_prefix "\${pcfiledir}")
		string(REPLACE "/" ";" parts "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
		foreach(part IN LISTS parts)
			if(NOT part STREQUAL "" AND NOT part STREQUAL ".")
				string(APPEND pc_prefix "/..")
			endif()
		endforeach()
		set(pc_libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
	endif()
	if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
		set(pc_includedir "${CMAKE_INSTALL_INCLUDEDIR}")
	else()
		set(pc_includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
	endif()

	get_target_property(type ${target} TYPE)
	set(pc_cflags "-I\${includedir}")
	set(pc_libs "-L\${libdir} -lraven")
	if(type STREQUAL "SHARED_LIBRARY")
		# what raven::raven gives the programs of a shared build (raven_core.h: dllimport on Windows)
		string(APPEND pc_cflags " -DRAVEN_USE_DLL")
		set(pc_libs_private "${private}")
	else()
		# no shared library: --libs alone must link libraven.a
		if(private)
			string(APPEND pc_libs " ${private}")
		endif()
		set(pc_libs_private "")
	endif()
	if(public_options)
		string(APPEND pc_libs " ${public_options}")
	endif()

	set(pc_requires "")
	if(requires)
		if(type STREQUAL "SHARED_LIBRARY")
			set(pc_requires "Requires.private: ${requires}\n")
		else()
			set(pc_requires "Requires: ${requires}\n")
		endif()
	endif()

	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/raven.pc"
"prefix=${pc_prefix}
libdir=${pc_libdir}
includedir=${pc_includedir}

Name: raven
Description: ${PROJECT_DESCRIPTION}
URL: https://github.com/oscar-soirey/Raven
Version: ${PROJECT_VERSION}
${pc_requires}Cflags: ${pc_cflags}
Libs: ${pc_libs}
Libs.private: ${pc_libs_private}
")
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/raven.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endfunction()
