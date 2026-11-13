## ADDED Requirements

### Requirement: Custom accessible actions and checked state
The library SHALL provide the following behavior while preserving existing defaults: Add RAccessiblePressCallback, Rwidget_set_accessible_action(widget, press, user_data), and Rwidget_set_accessible_checked(widget, checked). Explicit opt-in avoids inferring behavior from a role alone. NULL removes the action; checked accepts -1 (no override), 0 and 1.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When a custom button has an explicit accessible action and is pressed by assistive technology, its callback runs exactly once. Checked state changes are exposed through the shared accessibility node; clearing the override restores class-derived state.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
