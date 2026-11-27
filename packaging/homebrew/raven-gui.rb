# Homebrew formula for Raven (packaging/README.md: how to update it for a
# release and where to submit it). "raven" is already a formula of
# homebrew-core (another project), so this one is "raven-gui", like the
# packages of the bindings. Installed: the shared library and libraven.a,
# the headers, the CMake package (find_package(raven)), raven.pc
# (pkg-config raven), the designer and the theme editor.
class RavenGui < Formula
  desc "Lightweight multithreaded GUI library in C"
  homepage "https://github.com/oscar-soirey/Raven"
  # set_version.py writes the version; the sha256 is the one of this archive (packaging/README.md)
  url "https://github.com/oscar-soirey/Raven/archive/refs/tags/v0.3.0.tar.gz"
  sha256 "0000000000000000000000000000000000000000000000000000000000000000"
  license "Apache-2.0"
  head "https://github.com/oscar-soirey/Raven.git", branch: "main"

  depends_on "cmake" => [:build, :test]
  depends_on "vulkan-headers" => :build
  depends_on "pkgconf" => :test

  on_linux do
    depends_on "libx11"
  end

  def install
    args = %W[
      -DRAVEN_BUILD_TESTS=OFF
      -DRAVEN_BUILD_EXAMPLES=OFF
      -DRAVEN_FETCH_VULKAN_HEADERS=OFF
      -DRAVEN_VULKAN_INCLUDE_DIR=#{formula_opt_include("vulkan-headers")}
    ]

    # libraven.a first, then the shared library with the tools: the CMake
    # package and raven.pc that stay installed are those of the shared library
    system "cmake", "-S", ".", "-B", "build-static", "-DRAVEN_BUILD_SHARED=OFF", "-DRAVEN_BUILD_TOOLS=OFF",
           *args, *std_cmake_args
    system "cmake", "--build", "build-static"
    lib.install "build-static/libraven.a"

    system "cmake", "-S", ".", "-B", "build", "-DRAVEN_BUILD_SHARED=ON", "-DRAVEN_BUILD_TOOLS=ON",
           *args, *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    (testpath/"test.c").write <<~C
      #include <Raven.h>
      #include <stdio.h>
      #include <string.h>

      int main(void) {
        RInitDesc desc;
        memset(&desc, 0, sizeof(desc));
        desc.backend = R_BACKEND_NULL; /* no GPU */
        desc.headless = R_TRUE;        /* no window system */
        desc.worker_threads = -1;
        if (!Rinit_ex(&desc)) return 1;
        const char* xml = "<Window><Label Name=\\"hello\\" Text=\\"Hello\\"/></Window>";
        Rid window = Rwindow_create_from_xml(xml, strlen(xml));
        Rwait_frame(0.0);
        int ok = strcmp(Rlabel_get_text(Rwidget_find(window, "hello")), "Hello") == 0;
        printf("Raven %s\\n", Rget_version());
        Rwindow_destroy(window);
        Rshutdown();
        return ok ? 0 : 1;
      }
    C

    # pkg-config
    flags = shell_output("#{formula_opt_bin("pkgconf")}/pkgconf --cflags --libs raven").chomp.split
    system ENV.cc, "test.c", "-o", "test_pkgconfig", *flags
    assert_match "Raven #{version}", shell_output("./test_pkgconfig") unless build.head?

    # libraven.a, with what pkg-config --static says it needs
    static = shell_output("#{formula_opt_bin("pkgconf")}/pkgconf --cflags --libs --static raven").chomp.split
    static = static.map { |flag| (flag == "-lraven") ? (lib/"libraven.a").to_s : flag }
    system ENV.cc, "test.c", "-o", "test_static", *static
    system "./test_static"

    # CMake: find_package(raven)
    (testpath/"CMakeLists.txt").write <<~CMAKE
      cmake_minimum_required(VERSION 3.16)
      project(test_raven C)
      find_package(raven REQUIRED)
      add_executable(test_cmake test.c)
      target_link_libraries(test_cmake PRIVATE raven::raven)
    CMAKE
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "./build/test_cmake"
  end
end
