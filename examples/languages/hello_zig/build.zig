const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    // (a copy of bindings/zig of Raven: a build.zig only imports files of its project)
    const raven = b.createModule(.{ .root_source_file = b.path("raven/raven.zig"), .target = target, .optimize = optimize });
    const exe = b.addExecutable(.{
        .name = "hello_zig",
        .root_module = b.createModule(.{ .root_source_file = b.path("src/main.zig"), .target = target, .optimize = optimize }),
    });
    exe.root_module.addImport("raven", raven);
    exe.root_module.linkSystemLibrary("raven", .{});
    exe.root_module.link_libc = true;
    if (b.graph.env_map.get("RAVEN_LIB_DIR")) |dir| exe.root_module.addLibraryPath(.{ .cwd_relative = dir });
    b.installArtifact(exe);
    const run = b.addRunArtifact(exe);
    // main.xml and strings.csv are read from the folder of the project
    run.setCwd(b.path("."));
    b.step("run", "Run the program").dependOn(&run.step);
}
