## Context
Custom widgets have no invokable state or action path; application event callbacks are bypassed by the existing synthetic-key helper.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add RAccessiblePressCallback, Rwidget_set_accessible_action(widget, press, user_data), and Rwidget_set_accessible_checked(widget, checked). Explicit opt-in avoids inferring behavior from a role alone. NULL removes the action; checked accepts -1 (no override), 0 and 1.

Extend the lazily allocated rvn_a11y_t in src/core/accessibility.c. Apply explicit state overrides after class-derived states in rvn_a11y_describe. Invoke explicit callbacks for assistive press/toggle actions within rvn_dispatch_begin/end, honoring inherited disabled state and avoiding access after callbacks may destroy a widget. Existing controls keep their class behavior unless explicitly overridden. Notify platform state changes when checked changes. Cocoa AXPress/AXValue, AT-SPI actions/states, UIA Invoke/Toggle and web ARIA must reflect the common description.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
