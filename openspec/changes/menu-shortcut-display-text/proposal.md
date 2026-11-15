# Display shortcut hints independently of bindings

## Why
Rmenu_item_set_shortcut currently couples the visible label to keyboard activation, and native menus suppress labels for unmodified printable keys.

Issue: https://github.com/oscar-soirey/Raven/issues/94

## What Changes
Add Rmenu_item_set_shortcut_text(item, text), a copied display-only override. NULL clears the override and restores the formatted bound shortcut. Setting a label never binds or unbinds a key; a separately set binding continues to work.

## Capabilities

### New Capabilities
- `menu-shortcut-display-text`: Display shortcut hints independently of bindings.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #94; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
