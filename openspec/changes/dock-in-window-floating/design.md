## Context
DockArea floating always creates an OS window. Overlay cards implemented by applications lose docking previews and layout persistence.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add RDockFloatMode with R_DOCK_FLOAT_WINDOW (default) and R_DOCK_FLOAT_IN_WINDOW, and Rdock_area_set_float_mode. Preserve Rdock_area_float_panel return semantics by returning the containing window in in-window mode. Existing floating panels retain their mode when the default changes.

Extend src/widgets/dock_area.c with a separate ordered list of floating groups and logical rectangles under the home area. Use existing tab painting, dragging, group activation and drop previews. Paint floating groups after docked content and hit-test in reverse order; clip and clamp them to the containing window. Represent each card as a child DockArea in the existing floating family, owning its panels. Register cards with an internal window paint/hit-test layer above root content and below popups, with no extra OS windows. Save a versioned extension containing floating groups and geometry while accepting old layouts and leaving dock-only serialization unchanged. Restore atomically after validating names, bounds and duplicate panels. Dock, hide, close and destruction must remove the floating membership exactly once.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
