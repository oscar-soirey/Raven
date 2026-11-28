# vcpkg port of Raven (packaging/README.md: how to update it for a release,
# test it from a local tree, and submit it). "raven-gui" like the packages of
# the bindings; the CMake package is find_package(raven), pkg-config raven.

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO oscar-soirey/Raven
    REF "v${VERSION}"
    # the SHA512 of the archive of the tag: packaging/README.md says how to get it
    SHA512 0
    HEAD_REF main
)

if(VCPKG_TARGET_IS_LINUX)
    message(STATUS "raven-gui needs the X11 development files of the system:\n    sudo apt install libx11-dev   (Debian, Ubuntu)\n    sudo dnf install libX11-devel (Fedora)")
endif()

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" RAVEN_SHARED)
string(COMPARE EQUAL "${VCPKG_CRT_LINKAGE}" "static" RAVEN_STATIC_CRT)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools RAVEN_BUILD_TOOLS
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DRAVEN_BUILD_SHARED=${RAVEN_SHARED}
        -DRAVEN_MSVC_STATIC_RUNTIME=${RAVEN_STATIC_CRT}
        -DRAVEN_BUILD_TESTS=OFF
        -DRAVEN_BUILD_EXAMPLES=OFF
        # the Vulkan headers of the port vulkan-headers, nothing downloaded
        -DRAVEN_FETCH_VULKAN_HEADERS=OFF
        "-DRAVEN_VULKAN_INCLUDE_DIR=${CURRENT_INSTALLED_DIR}/include"
    MAYBE_UNUSED_VARIABLES
        RAVEN_MSVC_STATIC_RUNTIME
)

vcpkg_cmake_install()

# share/raven of the install holds the README, the docs, the bindings and the licenses:
# the licenses go to the copyright file, the rest is not for vcpkg
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/raven" "${CURRENT_PACKAGES_DIR}/debug/share/raven")

vcpkg_cmake_config_fixup(PACKAGE_NAME raven CONFIG_PATH lib/cmake/raven)
vcpkg_fixup_pkgconfig()

if("tools" IN_LIST FEATURES)
    vcpkg_copy_tools(TOOL_NAMES raven_designer raven_theme_editor AUTO_CLEAN)
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE" "${SOURCE_PATH}/NOTICE" "${SOURCE_PATH}/THIRD_PARTY.md")
