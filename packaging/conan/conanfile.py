"""
Conan 2 recipe of Raven (packaging/README.md: how to update it for a release,
test it from a local tree, and submit it to Conan Center).

    conan create packaging/conan --build=missing
    conan create packaging/conan --build=missing -o "&:shared=True" -o "&:tools=True"

"raven-gui" like the packages of the bindings; the CMake package is
find_package(raven) + raven::raven, the pkg-config module raven.
"""

import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get, rmdir
from conan.tools.microsoft import is_msvc, is_msvc_static_runtime

required_conan_version = ">=2.0"


class RavenGuiConan(ConanFile):
    name = "raven-gui"
    # written by bindings/set_version.py; the sources of each version are in conandata.yml
    version = "0.3.0"
    description = "Raven: a lightweight multithreaded GUI library in C, drawn with Vulkan or Metal, interfaces in XML."
    license = "Apache-2.0"
    url = "https://github.com/oscar-soirey/Raven"
    homepage = "https://github.com/oscar-soirey/Raven"
    topics = ("gui", "ui", "widgets", "vulkan", "metal", "xml", "c")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "tools": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "tools": False,
    }
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        # a C library
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        # raven/raven_vulkan.h includes vulkan/vulkan.h
        self.requires("vulkan-headers/1.3.290.0", transitive_headers=True)

    def validate(self):
        if self.settings.os not in ("Windows", "Linux", "Macos"):
            raise ConanInvalidConfiguration("raven-gui: this recipe builds for Windows, Linux and macOS")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["RAVEN_BUILD_SHARED"] = bool(self.options.shared)
        tc.cache_variables["RAVEN_BUILD_TOOLS"] = bool(self.options.tools)
        tc.cache_variables["RAVEN_BUILD_TESTS"] = False
        tc.cache_variables["RAVEN_BUILD_EXAMPLES"] = False
        tc.cache_variables["RAVEN_FETCH_VULKAN_HEADERS"] = False
        vulkan = self.dependencies["vulkan-headers"].cpp_info
        tc.cache_variables["RAVEN_VULKAN_INCLUDE_DIR"] = vulkan.includedirs[0].replace("\\", "/")
        if is_msvc(self):
            tc.cache_variables["RAVEN_MSVC_STATIC_RUNTIME"] = is_msvc_static_runtime(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        for name in ("LICENSE", "NOTICE", "THIRD_PARTY.md"):
            copy(self, name, self.source_folder, os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        # Conan writes its own CMake and pkg-config files (package_info); the README, docs and bindings are not for it
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "raven")
        self.cpp_info.set_property("cmake_target_name", "raven::raven")
        self.cpp_info.set_property("pkg_config_name", "raven")
        self.cpp_info.libs = ["raven"]
        if self.options.shared:
            self.cpp_info.defines = ["RAVEN_USE_DLL"]
        if self.settings.os == "Linux":
            self.cpp_info.system_libs = ["X11", "m", "dl", "pthread"]
        elif self.settings.os == "Macos":
            self.cpp_info.frameworks = ["CoreFoundation", "AppKit", "QuartzCore", "Metal", "UserNotifications", "Foundation"]
        elif self.settings.os == "Windows":
            self.cpp_info.system_libs = ["winhttp", "user32", "gdi32", "shell32", "comdlg32", "ole32"]
