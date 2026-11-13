## Context
Topmost applies across applications and embedded platform child views are not owned top-level windows.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add Rwindow_set_owner(window, owner) and Rwindow_get_owner(window). R_INVALID_ID detaches ownership. Reject self ownership and cycles. Ownership is non-modal; it does not imply global topmost.

Store owner Rid in rvn_window_t and add an optional platform ownership hook to src/platform/platform.h. Use Cocoa addChildWindow/removeChildWindow with normal window levels, Win32 GWLP_HWNDPARENT plus reversible WS_EX_TOOLWINDOW state, X11 transient-for and utility type, and Wayland xdg_toplevel parent. Synchronize owner visibility/minimization through core state handling where the compositor does not do it. Preserve each tool's explicit visibility so restoring an owner does not reveal a deliberately hidden tool. Detach children before owner destruction rather than deleting application-owned windows. Null/host/web retain logical ownership with documented platform limits.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
