// Raven for Zig: every declaration of raven.zig compiles with the Zig at hand
// (ctest binding_zig: `zig test check.zig --test-no-exec -fno-emit-bin`). Zig
// checks every declaration and makes no binary: nothing is linked or run, so
// neither the library nor a GPU is needed. (--test-no-exec alone still links the
// test binary, and every function of the library is then an undefined symbol.)

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
