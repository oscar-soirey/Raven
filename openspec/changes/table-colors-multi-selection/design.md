## Context
Table has a single selected index and theme-wide text color, while TreeView equivalents cannot replace virtual multi-column data.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add the cell-color setter and virtual color callback proposed in #97, plus set_multi_select, set_row_selected, is_row_selected, clear_selection, get_selection_count and get_selection. Single selection remains the default and Rtable_get_selected remains the primary row.

Extend src/widgets/table.c with selection membership plus anchor/primary indices. Keep allocation proportional to rows rather than columns for selection; allocate stored color overrides lazily. Cell color overrides row color, which overrides theme; alpha zero clears an override. Virtual callback returning false uses the theme. Explicit colors apply to selected-row text as well. Paint callbacks must stay on the UI thread as text callbacks do. Ctrl/Cmd click toggles membership; Shift click/arrows selects anchored ranges; ordinary selection replaces membership. Stable stored sorting permutes membership, colors, anchor and primary together. Removing rows and switching data modes must reconcile all indices; virtual sort follows the existing application-owned data ordering contract. Extend shared accessibility with table row nodes/multiple selection metadata and map Cocoa selected rows, AT-SPI selected states, UIA CanSelectMultiple and web ARIA.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
