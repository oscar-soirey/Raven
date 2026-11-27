# A project of its own that uses an installed Raven (package_consumer.cmake,
# the Homebrew formula, the vcpkg port, the Conan recipe):
#
#   cmake -S tests/package -B build-consumer -DCMAKE_PREFIX_PATH=<prefix of Raven>
cmake_minimum_required(VERSION 3.16)
project(raven_consumer LANGUAGES C)

find_package(raven REQUIRED)
add_executable(raven_consumer consumer.c)
target_link_libraries(raven_consumer PRIVATE raven::raven)
