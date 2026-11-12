# Raven SDK Installer - writes a C++ file that holds files as arrays (the XML of the
# interface, the logo), so that the installer needs no file beside it.
#
#   cmake -DOUTPUT=resources.cpp -DPREFIX=ui -DFILES="a.xml;b.png" -P EmbedResources.cmake
#
# Each file is registered at <PREFIX>/<its name> (Rfile_register_memory).

if(NOT OUTPUT OR NOT FILES)
	message(FATAL_ERROR "EmbedResources.cmake: OUTPUT and FILES are needed")
endif()

set(_body "// Written by tools/installer/cmake/EmbedResources.cmake: do not edit.\n#include \"resources.hpp\"\n\nnamespace rinst::resources {\n\nnamespace {\n")
set(_table "")
set(_index 0)
foreach(_file IN LISTS FILES)
	get_filename_component(_name "${_file}" NAME)
	file(READ "${_file}" _hex HEX)
	string(LENGTH "${_hex}" _length)
	math(EXPR _size "${_length} / 2")
	# 0xAB, every byte; a line break every 32 bytes
	string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
	string(REGEX REPLACE "((0x[0-9a-f][0-9a-f],){32})" "\\1\n\t" _bytes "${_bytes}")
	string(APPEND _body "// ${_name}\nconst unsigned char file_${_index}[] = {\n\t${_bytes}0x00\n};\n")
	string(APPEND _table "\t{\"${PREFIX}/${_name}\", file_${_index}, ${_size}},\n")
	math(EXPR _index "${_index} + 1")
endforeach()
string(APPEND _body "} // namespace\n\nconst Resource kResources[] = {\n${_table}};\nconst std::size_t kResourceCount = ${_index};\n\n} // namespace rinst::resources\n")

# rewritten only when it changes: no rebuild for nothing
if(EXISTS "${OUTPUT}")
	file(READ "${OUTPUT}" _old)
	if(_old STREQUAL _body)
		return()
	endif()
endif()
file(WRITE "${OUTPUT}" "${_body}")
