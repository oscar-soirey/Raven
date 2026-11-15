## ADDED Requirements

### Requirement: Choose the drop highlight rectangle
The library SHALL provide the following behavior while preserving existing defaults: Add Rdrop_set_highlight_rect(x, y, width, height), usable only during R_DROP_ENTER/R_DROP_MOVE. Coordinates are local logical units of the callback target (window-local for window callbacks). Zero width or height suppresses the highlight without changing acceptance.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When an accepting target chooses a smaller rectangle, Raven highlights that rectangle and still delivers the drop. Choosing zero size hides the highlight while acceptance remains true; a later callback without customization restores the default.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
