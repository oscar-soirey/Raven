# Table cell colors and multiple row selection

## Why
Table has a single selected index and theme-wide text color, while TreeView equivalents cannot replace virtual multi-column data.

Issue: https://github.com/oscar-soirey/Raven/issues/97

## What Changes
Add the cell-color setter and virtual color callback proposed in #97, plus set_multi_select, set_row_selected, is_row_selected, clear_selection, get_selection_count and get_selection. Single selection remains the default and Rtable_get_selected remains the primary row.

## Capabilities

### New Capabilities
- `table-colors-multi-selection`: Table cell colors and multiple row selection.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #97; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
