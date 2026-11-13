## Context
An accepted drop is always highlighted over the whole target; refusing the highlight also refuses the drop.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add Rdrop_set_highlight_rect(x, y, width, height), usable only during R_DROP_ENTER/R_DROP_MOVE. Coordinates are local logical units of the callback target (window-local for window callbacks). Zero width or height suppresses the highlight without changing acceptance.

Keep a scoped active-drop context around rvn_window_drop_send in src/widget/window.c, restoring the previous context after nested dispatch. Reset custom rectangle state for every ENTER/MOVE so omitted customization restores the default. Clamp finite nonnegative rectangles to target bounds, translate to window coordinates, and separate highlight visibility from drop_accepted. Reset animation when the visible rectangle changes; quiet acceptance must not leave a fading full-target highlight. Reject calls outside eligible callbacks and non-finite/negative dimensions using existing error conventions. Leave callback return semantics unchanged.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
