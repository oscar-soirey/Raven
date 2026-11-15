# Choose the drop highlight rectangle

## Why
An accepted drop is always highlighted over the whole target; refusing the highlight also refuses the drop.

Issue: https://github.com/oscar-soirey/Raven/issues/95

## What Changes
Add Rdrop_set_highlight_rect(x, y, width, height), usable only during R_DROP_ENTER/R_DROP_MOVE. Coordinates are local logical units of the callback target (window-local for window callbacks). Zero width or height suppresses the highlight without changing acceptance.

## Capabilities

### New Capabilities
- `custom-drop-highlight-rect`: Choose the drop highlight rectangle.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #95; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
