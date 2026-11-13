## ADDED Requirements

### Requirement: Table cell colors and multiple row selection
The library SHALL provide the following behavior while preserving existing defaults: Add the cell-color setter and virtual color callback proposed in #97, plus set_multi_select, set_row_selected, is_row_selected, clear_selection, get_selection_count and get_selection. Single selection remains the default and Rtable_get_selected remains the primary row.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When multi-selection is enabled, modifier clicks and Shift navigation select multiple rows and enumeration returns them top to bottom. Stored sorting keeps selected rows and color overrides with their data; default tables retain single selection.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
