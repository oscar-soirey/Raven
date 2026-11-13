# Owned non-modal tool windows

## Why
Topmost applies across applications and embedded platform child views are not owned top-level windows.

Issue: https://github.com/oscar-soirey/Raven/issues/93

## What Changes
Add Rwindow_set_owner(window, owner) and Rwindow_get_owner(window). R_INVALID_ID detaches ownership. Reject self ownership and cycles. Ownership is non-modal; it does not imply global topmost.

## Capabilities

### New Capabilities
- `owned-tool-windows`: Owned non-modal tool windows.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #93; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
