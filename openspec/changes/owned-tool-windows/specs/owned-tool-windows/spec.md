## ADDED Requirements

### Requirement: Owned non-modal tool windows
The library SHALL provide the following behavior while preserving existing defaults: Add Rwindow_set_owner(window, owner) and Rwindow_get_owner(window). R_INVALID_ID detaches ownership. Reject self ownership and cycles. Ownership is non-modal; it does not imply global topmost.

#### Scenario: Application uses the new capability
- **WHEN** the application configures this capability through the public API
- **THEN** When a tool has an owner it stays above that owner without becoming globally topmost and has no independent taskbar entry where supported. Hiding/minimizing its owner hides it; detaching restores ordinary window behavior.

#### Scenario: Application keeps existing defaults
- **WHEN** the application does not opt into the new API
- **THEN** existing behavior SHALL remain compatible
