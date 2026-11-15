## Context
Rmenu_item_set_shortcut currently couples the visible label to keyboard activation, and native menus suppress labels for unmodified printable keys.

## Goals / Non-Goals
Implement the issue behavior with the existing platform/widget contracts and backwards-compatible defaults. Do not redesign unrelated APIs or add external runtime dependencies.

## Decisions
Add Rmenu_item_set_shortcut_text(item, text), a copied display-only override. NULL clears the override and restores the formatted bound shortcut. Setting a label never binds or unbinds a key; a separately set binding continues to work.

Keep an explicit hint separate from the formatted shortcut in rvn_menu_item_t in src/widgets/menu.c. Share a display-text helper between measuring and painting and pass the explicit hint through rvn_pmenu_t. For Cocoa use an attributed title with a right-aligned tab stop; never derive a keyEquivalent from the hint. Preserve separately configured key/modifier behavior. Invalidate layout and rebuild native menus on updates. Add ShortcutText XML schema/attribute support consistent with existing Shortcut.

## Risks / Trade-offs
Callbacks may delete widgets/windows: use existing dispatch guards and re-resolve handles. Native APIs differ; document limitations and never claim unexecuted platform checks passed. Regenerate all supported bindings after header changes and run their freshness check.

## Validation
Use CTest regression tests and the null renderer, native platform tests where available, binding freshness, and supported platform CI builds. Compare any failures with main before assigning origin.

## Review Gate
Proposal, design and tasks require user review before implementation, per the supplied coding guidance.
