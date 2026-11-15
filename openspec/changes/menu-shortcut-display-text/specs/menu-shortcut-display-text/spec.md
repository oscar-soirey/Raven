## ADDED Requirements

### Requirement: Display shortcut hints independently of bindings
The library SHALL provide the following behavior while preserving existing defaults: Add Rmenu_item_set_shortcut_text(item, text), a copied display-only override. NULL clears the override and restores the formatted bound shortcut. Setting a label never binds or unbinds a key; a separately set binding continues to work.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When an item has only a display hint, its menu shows that hint and typing it does not activate the item. When an explicit hint is cleared, an existing bound shortcut is shown again.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
