## 1. Review
- [x] 1.1 Review proposal, design and tasks with the user before implementation (approved 2026-11-13).

## 2. Implementation
- [x] 2.1 Add API declaration and scoped callback context with argument validation.
- [x] 2.2 Apply local rectangles in drag tracking; separate visual state from acceptance and preserve default behavior.
- [x] 2.3 Handle target changes, quiet highlights, rejected drops, nested callbacks and target destruction.
- [x] 2.4 Test accepted quiet drops, custom geometry, default reset, scaling, invalid calls, cancellation and transitions between targets.

## 3. Verification and contribution
- [x] 3.1 Regenerate bindings and update public API/behavior documentation.
- [x] 3.2 Run focused regressions, applicable CTest suite and binding freshness; compare failures with main.
- [x] 3.3 Review all changes against Raven architecture and contribution rules.
- [ ] 3.4 Create an independent PR referencing and closing #95, documenting verification and platform limitations.
