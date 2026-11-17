# Menu bar dividers, title colors and caption rows

## Why
MenuBar only draws uniformly colored titles and Menu has no noninteractive caption entry.

Issue: https://github.com/oscar-soirey/Raven/issues/96

## What Changes
Add Rmenu_bar_add_divider, Rmenu_set_title_color and Rmenu_add_header. Alpha zero restores the theme title color. Dividers and headers are noninteractive and skipped by keyboard navigation.

## Capabilities

### New Capabilities
- `menu-dividers-tints-headers`: Menu bar dividers, title colors and caption rows.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #96; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
