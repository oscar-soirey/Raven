// Raven for Zig: every declaration of raven.zig compiles with the Zig at hand
// (ctest binding_zig: `zig test check.zig --test-no-exec`). Nothing is linked or
// run, so neither the library nor a GPU is needed.

const std = @import("std");
const r = @import("raven.zig");

test "raven.zig compiles" {
    @setEvalBranchQuota(100000);
    std.testing.refAllDecls(r);
    // a structure written `.{}` is the zeroed structure of C, arrays included
    const desc: r.RInitDesc = .{};
    const tile: r.RTileItem = .{};
    try std.testing.expectEqual(@as(c_int, 0), desc.backend);
    try std.testing.expectEqual(@as(f32, 0), tile.uv[3]);
}
