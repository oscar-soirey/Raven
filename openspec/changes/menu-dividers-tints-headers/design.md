## Context
MenuBar only draws uniformly colored titles and Menu has no noninteractive caption entry.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add Rmenu_bar_add_divider, Rmenu_set_title_color and Rmenu_add_header. Alpha zero restores the theme title color. Dividers and headers are noninteractive and skipped by keyboard navigation.

Reuse menu-item storage with explicit divider/header entry kinds in src/widgets/menu.c. Make bar measurement, painting, hit testing and keyboard movement handle dividers safely. Store title color on Menu independently from item text colors. Render headers as secondary text and expose them as static/heading accessibility nodes with no press action. Extend native menu projection so Cocoa omits bar dividers/title tints and uses sectionHeaderWithTitle on macOS 14+, with a disabled no-action fallback on older versions. Propagate updates to native bars and tray/dock menus where appropriate.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
