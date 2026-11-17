## ADDED Requirements

### Requirement: Menu bar dividers, title colors and caption rows
The library SHALL provide the following behavior while preserving existing defaults: Add Rmenu_bar_add_divider, Rmenu_set_title_color and Rmenu_add_header. Alpha zero restores the theme title color. Dividers and headers are noninteractive and skipped by keyboard navigation.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When plugin menus are separated by a divider and tinted, the in-window bar displays those decorations. A caption row is visible but never activated or focused, and native macOS menus use supported section-header behavior.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
