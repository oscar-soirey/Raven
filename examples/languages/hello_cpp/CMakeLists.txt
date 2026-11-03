cmake_minimum_required(VERSION 3.16)
project(hello_cpp CXX)

# Raven: the package (bin/, include/, lib/). The IDE gives its folder in CMAKE_PREFIX_PATH.
find_package(raven REQUIRED)

# main.xml (the interface) and strings.csv (the translations) are read when the program
# runs, from its working folder: the IDE runs it in the folder of the project.
add_executable(hello_cpp main.cpp main.xml strings.csv)
target_link_libraries(hello_cpp PRIVATE raven::raven)
if(MSVC)
	target_compile_definitions(hello_cpp PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
