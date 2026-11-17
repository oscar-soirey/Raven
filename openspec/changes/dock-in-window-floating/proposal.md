# Float dock panels within their existing window

## Why
DockArea floating always creates an OS window. Overlay cards implemented by applications lose docking previews and layout persistence.

Issue: https://github.com/oscar-soirey/Raven/issues/92

## What Changes
Add RDockFloatMode with R_DOCK_FLOAT_WINDOW (default) and R_DOCK_FLOAT_IN_WINDOW, and Rdock_area_set_float_mode. Preserve Rdock_area_float_panel return semantics by returning the containing window in in-window mode. Existing floating panels retain their mode when the default changes.

## Capabilities

### New Capabilities
- `dock-in-window-floating`: Float dock panels within their existing window.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #92; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
