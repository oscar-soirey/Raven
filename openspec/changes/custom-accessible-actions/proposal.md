# Custom accessible actions and checked state

## Why
Custom widgets have no invokable state or action path; application event callbacks are bypassed by the existing synthetic-key helper.

Issue: https://github.com/oscar-soirey/Raven/issues/91

## What Changes
Add RAccessiblePressCallback, Rwidget_set_accessible_action(widget, press, user_data), and Rwidget_set_accessible_checked(widget, checked). Explicit opt-in avoids inferring behavior from a role alone. NULL removes the action; checked accepts -1 (no override), 0 and 1.

## Capabilities

### New Capabilities
- `custom-accessible-actions`: Custom accessible actions and checked state.

### Modified Capabilities
None: Raven has no existing OpenSpec baseline.

## Impact
One independent implementation PR for #91; retain C11, Raven handle validation and layer boundaries. Public headers require regenerated language bindings, documentation, and meaningful regression coverage. No unrelated widget or renderer redesign.
