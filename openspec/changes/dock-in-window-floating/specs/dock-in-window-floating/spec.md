## ADDED Requirements

### Requirement: Float dock panels within their existing window
The library SHALL provide the following behavior while preserving existing defaults: Add RDockFloatMode with R_DOCK_FLOAT_WINDOW (default) and R_DOCK_FLOAT_IN_WINDOW, and Rdock_area_set_float_mode. Preserve Rdock_area_float_panel return semantics by returning the containing window in in-window mode. Existing floating panels retain their mode when the default changes.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When in-window float mode is enabled, floating a panel keeps it above docked content in the same window. Dragging its tab can move it and dock it again; saving and loading restores its membership and geometry.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
